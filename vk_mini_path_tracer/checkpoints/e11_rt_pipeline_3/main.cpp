// Copyright 2020 NVIDIA Corporation
// SPDX-License-Identifier: Apache-2.0
#include <array>
#include <random>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <fileformats/stb_image_write.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <fileformats/tiny_obj_loader.h>
#include <nvh/fileoperations.hpp>  // For nvh::loadFile
#define NVVK_ALLOC_DEDICATED
#include <nvvk/allocator_vk.hpp>  // For NVVK memory allocators
#include <nvvk/context_vk.hpp>
#include <nvvk/descriptorsets_vk.hpp>  // For nvvk::DescriptorSetContainer
#include <nvvk/raytraceKHR_vk.hpp>     // For nvvk::RaytracingBuilderKHR
#include <nvvk/shaders_vk.hpp>         // For nvvk::createShaderModule
#include <nvvk/structs_vk.hpp>         // For nvvk::make

#include "common.h"

PushConstants  pushConstants;
const uint32_t render_width  = 800;
const uint32_t render_height = 600;

VkCommandBuffer AllocateAndBeginOneTimeCommandBuffer(VkDevice device, VkCommandPool cmdPool)
{
  VkCommandBufferAllocateInfo cmdAllocInfo = nvvk::make<VkCommandBufferAllocateInfo>();
  cmdAllocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandPool                 = cmdPool;
  cmdAllocInfo.commandBufferCount          = 1;
  VkCommandBuffer cmdBuffer;
  NVVK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmdBuffer));
  VkCommandBufferBeginInfo beginInfo = nvvk::make<VkCommandBufferBeginInfo>();
  beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  NVVK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
  return cmdBuffer;
}

void EndSubmitWaitAndFreeCommandBuffer(VkDevice device, VkQueue queue, VkCommandPool cmdPool, VkCommandBuffer& cmdBuffer)
{
  NVVK_CHECK(vkEndCommandBuffer(cmdBuffer));
  VkSubmitInfo submitInfo       = nvvk::make<VkSubmitInfo>();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &cmdBuffer;
  NVVK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  NVVK_CHECK(vkQueueWaitIdle(queue));
  vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
}

VkDeviceAddress GetBufferDeviceAddress(VkDevice device, VkBuffer buffer)
{
  VkBufferDeviceAddressInfo addressInfo = nvvk::make<VkBufferDeviceAddressInfo>();
  addressInfo.buffer                    = buffer;
  return vkGetBufferDeviceAddress(device, &addressInfo);
}

int main(int argc, const char** argv)
{
  // Create the Vulkan context, consisting of an instance, device, physical device, and queues.
  nvvk::ContextCreateInfo deviceInfo;  // One can modify this to load different extensions or pick the Vulkan core version
  deviceInfo.apiMajor = 1;             // Specify the version of Vulkan we'll use
  deviceInfo.apiMinor = 2;
  // Required by KHR_acceleration_structure; allows work to be offloaded onto background threads and parallelized
  deviceInfo.addDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
  VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures = nvvk::make<VkPhysicalDeviceAccelerationStructureFeaturesKHR>();
  deviceInfo.addDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false, &asFeatures);
  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures = nvvk::make<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>();
  deviceInfo.addDeviceExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, false, &rtPipelineFeatures);

  nvvk::Context context;     // Encapsulates device state in a single object
  context.init(deviceInfo);  // Initialize the context
  // Device must support acceleration structures and ray tracing pipelines:
  assert(asFeatures.accelerationStructure == VK_TRUE && rtPipelineFeatures.rayTracingPipeline == VK_TRUE);

  // Get the properties of ray tracing pipelines on this device. We do this by
  // using vkGetPhysicalDeviceProperties2, and extending this by chaining on a
  // VkPhysicalDeviceRayTracingPipelinePropertiesKHR object to get both
  // physical device properties and ray tracing pipeline properties.
  // This gives us information about shader binding tables.
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties =
      nvvk::make<VkPhysicalDeviceRayTracingPipelinePropertiesKHR>();
  VkPhysicalDeviceProperties2 physicalDeviceProperties = nvvk::make<VkPhysicalDeviceProperties2>();
  physicalDeviceProperties.pNext                       = &rtPipelineProperties;
  vkGetPhysicalDeviceProperties2(context.m_physicalDevice, &physicalDeviceProperties);
  const VkDeviceSize sbtHeaderSize      = rtPipelineProperties.shaderGroupHandleSize;
  const VkDeviceSize sbtBaseAlignment   = rtPipelineProperties.shaderGroupBaseAlignment;
  const VkDeviceSize sbtHandleAlignment = rtPipelineProperties.shaderGroupHandleAlignment;

  // Compute the stride between shader binding table (SBT) records.
  // This must be:
  // - Greater than rtPipelineProperties.shaderGroupHandleSize (since a record
  //     contains a shader group handle)
  // - A multiple of rtPipelineProperties.shaderGroupHandleAlignment
  // - Less than or equal to rtPipelineProperties.maxShaderGroupStride
  // In addition, each SBT must start at a multiple of
  // rtPipelineProperties.shaderGroupBaseAlignment.
  // Since we store all records contiguously in a single SBT, we assert that
  // sbtBaseAlignment is a multiple of sbtHandleAlignment, round sbtHeaderSize
  // up to a multiple of sbtBaseAlignment, and then assert that the result is
  // less than or equal to maxShaderGroupStride.
  assert(sbtBaseAlignment % sbtHandleAlignment == 0);
  const VkDeviceSize sbtStride = sbtBaseAlignment *  //
                                 ((sbtHeaderSize + sbtBaseAlignment - 1) / sbtBaseAlignment);
  assert(sbtStride <= rtPipelineProperties.maxShaderGroupStride);

  // Initialize the debug utilities:
  nvvk::DebugUtil debugUtil(context);

  // Create the allocator
  nvvk::AllocatorDedicated allocator;
  allocator.init(context, context.m_physicalDevice);

  // Create an image. Images are more complex than buffers - they can have
  // multiple dimensions, different color+depth formats, be arrays of mips,
  // have multisampling, be tiled in memory in e.g. row-linear order or in an
  // implementation-dependent way (and this layout of memory can depend on
  // what the image is being used for), and be shared across multiple queues.
  // Here's how we specify the image we'll use:
  VkImageCreateInfo imageCreateInfo = nvvk::make<VkImageCreateInfo>();
  imageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;
  // RGB32 images aren't usually supported, so we change this to a RGBA32 image.
  imageCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  // Defines the size of the image:
  imageCreateInfo.extent = {render_width, render_height, 1};
  // The image is an array of length 1, and each element contains only 1 mip:
  imageCreateInfo.mipLevels   = 1;
  imageCreateInfo.arrayLayers = 1;
  // We aren't using MSAA (i.e. the image only contains 1 sample per pixel -
  // note that this isn't the same use of the word "sample" as in ray tracing):
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  // The driver controls the tiling of the image for performance:
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  // This image is read and written on the GPU, and data can be transferred
  // from it:
  imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  // Image is only used by one queue:
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // The image must be in either VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED
  // according to the specification; we'll transition the layout shortly,
  // in the same command buffer used to upload the vertex and index buffers:
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  nvvk::ImageDedicated image    = allocator.createImage(imageCreateInfo);
  debugUtil.setObjectName(image.image, "image");

  // Create an image view for the entire image
  // When we create a descriptor for the image, we'll also need an image view
  // that the descriptor will point to. This specifies what part of the image
  // the descriptor views, and how the descriptor views it.
  VkImageViewCreateInfo imageViewCreateInfo = nvvk::make<VkImageViewCreateInfo>();
  imageViewCreateInfo.image                 = image.image;
  imageViewCreateInfo.viewType              = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCreateInfo.format                = imageCreateInfo.format;
  // We could use imageViewCreateInfo.components to make the components of the
  // image appear to be "swizzled", but we don't want to do that. Luckily,
  // all values are set to VK_COMPONENT_SWIZZLE_IDENTITY, which means
  // "don't change anything", by nvvk::make or zero initialization.
  // This says that the ImageView views the color part of the image (since
  // images can contain depth or stencil aspects):
  imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  // This says that we only look at array layer 0 and mip level 0:
  imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  imageViewCreateInfo.subresourceRange.layerCount     = 1;
  imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
  imageViewCreateInfo.subresourceRange.levelCount     = 1;
  VkImageView imageView;
  NVVK_CHECK(vkCreateImageView(context, &imageViewCreateInfo, nullptr, &imageView));
  debugUtil.setObjectName(imageView, "imageView");

  // Also create an image using linear tiling that can be accessed from the CPU,
  // much like how we created the buffer in the main tutorial. The first image
  // will be entirely local to the GPU for performance, while this image can
  // be mapped to CPU memory. We'll copy data from the first image to this
  // image in order to read the image data back on the CPU.
  // As before, we'll transition the image layout in the same command buffer
  // used to upload the vertex and index buffers.
  imageCreateInfo.tiling           = VK_IMAGE_TILING_LINEAR;
  imageCreateInfo.usage            = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  nvvk::ImageDedicated imageLinear = allocator.createImage(imageCreateInfo,                           //
                                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT       //
                                                               | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  //
                                                               | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
  debugUtil.setObjectName(imageLinear.image, "imageLinear");

  // Load the mesh of the first shape from an OBJ file
  std::vector<std::string> searchPaths = {
      PROJECT_ABSDIRECTORY,       PROJECT_ABSDIRECTORY "../",    PROJECT_ABSDIRECTORY "../../", PROJECT_RELDIRECTORY,
      PROJECT_RELDIRECTORY "../", PROJECT_RELDIRECTORY "../../", PROJECT_NAME};
  tinyobj::ObjReader       reader;  // Used to read an OBJ file
  reader.ParseFromFile(nvh::findFile("scenes/CornellBox-Original-Merged.obj", searchPaths));
  assert(reader.Valid());  // Make sure tinyobj was able to parse this file
  const std::vector<tinyobj::real_t>   objVertices = reader.GetAttrib().GetVertices();
  const std::vector<tinyobj::shape_t>& objShapes   = reader.GetShapes();  // All shapes in the file
  assert(objShapes.size() == 1);                                          // Check that this file has only one shape
  const tinyobj::shape_t& objShape = objShapes[0];                        // Get the first shape
  // Get the indices of the vertices of the first mesh of `objShape` in `attrib.vertices`:
  std::vector<uint32_t> objIndices;
  objIndices.reserve(objShape.mesh.indices.size());
  for(const tinyobj::index_t& index : objShape.mesh.indices)
  {
    objIndices.push_back(index.vertex_index);
  }

  // Create the command pool
  VkCommandPoolCreateInfo cmdPoolInfo = nvvk::make<VkCommandPoolCreateInfo>();
  cmdPoolInfo.queueFamilyIndex        = context.m_queueGCT;
  VkCommandPool cmdPool;
  NVVK_CHECK(vkCreateCommandPool(context, &cmdPoolInfo, nullptr, &cmdPool));
  debugUtil.setObjectName(cmdPool, "cmdPool");

  // Upload the vertex and index buffers to the GPU.
  nvvk::BufferDedicated vertexBuffer, indexBuffer;
  {
    // Start a command buffer for uploading the buffers
    VkCommandBuffer uploadCmdBuffer = AllocateAndBeginOneTimeCommandBuffer(context, cmdPool);
    // We get these buffers' device addresses, and use them as storage buffers and build inputs.
    const VkBufferUsageFlags usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                     | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    vertexBuffer = allocator.createBuffer(uploadCmdBuffer, objVertices, usage);
    indexBuffer  = allocator.createBuffer(uploadCmdBuffer, objIndices, usage);

    // Also, let's transition the layout of `image` to `VK_IMAGE_LAYOUT_GENERAL`,
    // and the layout of `imageLinear` to `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`.
    // Although we use `imageLinear` later, we're transferring its layout as
    // early as possible. For more complex applications, tracking images and
    // operations using a graph is a good way to handle these types of images
    // automatically. However, for this tutorial, we'll show how to write
    // image transitions by hand.

    // To do this, we combine both transitions in a single pipeline barrier.
    // This pipeline barrier will say "Make it so that all writes to memory by
    const VkAccessFlags srcAccesses = 0;  // Since image and imageLinear aren't initially accessible
    // finish and can be read correctly by
    const VkAccessFlags dstImageAccesses       = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;  // for image
    const VkAccessFlags dstImageLinearAccesses = VK_ACCESS_TRANSFER_WRITE_BIT;  // for imageLinear
    // "

    // Here's how to do that:
    const VkPipelineStageFlags srcStages = nvvk::makeAccessMaskPipelineStageFlags(srcAccesses);
    const VkPipelineStageFlags dstStages = nvvk::makeAccessMaskPipelineStageFlags(dstImageAccesses | dstImageLinearAccesses);
    VkImageMemoryBarrier       imageBarriers[2];
    // Image memory barrier for `image` from UNDEFINED to GENERAL layout:
    imageBarriers[0] = nvvk::makeImageMemoryBarrier(image.image,                    // The VkImage
                                                    srcAccesses, dstImageAccesses,  // Source and destination access masks
                                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,  // Source and destination layouts
                                                    VK_IMAGE_ASPECT_COLOR_BIT);  // Aspects of an image (color, depth, etc.)
    // Image memory barrier for `imageLinear` from UNDEFINED to TRANSFER_DST_OPTIMAL layout:
    imageBarriers[1] = nvvk::makeImageMemoryBarrier(imageLinear.image,                    // The VkImage
                                                    srcAccesses, dstImageLinearAccesses,  // Source and destination access masks
                                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // Source and dst layouts
                                                    VK_IMAGE_ASPECT_COLOR_BIT);  // Aspects of an image (color, depth, etc.)
    // Include the two image barriers in the pipeline barrier:
    vkCmdPipelineBarrier(uploadCmdBuffer,       // The command buffer
                         srcStages, dstStages,  // Src and dst pipeline stages
                         0,                     // Flags for memory dependencies
                         0, nullptr,            // Global memory barrier objects
                         0, nullptr,            // Buffer memory barrier objects
                         2, imageBarriers);     // Image barrier objects

    EndSubmitWaitAndFreeCommandBuffer(context, context.m_queueGCT, cmdPool, uploadCmdBuffer);
    allocator.finalizeAndReleaseStaging();
  }

  // Describe the bottom-level acceleration structure (BLAS)
  std::vector<nvvk::RaytracingBuilderKHR::BlasInput> blases;
  {
    nvvk::RaytracingBuilderKHR::BlasInput blas;
    // Get the device addresses of the vertex and index buffers
    VkDeviceAddress vertexBufferAddress = GetBufferDeviceAddress(context, vertexBuffer.buffer);
    VkDeviceAddress indexBufferAddress  = GetBufferDeviceAddress(context, indexBuffer.buffer);
    // Specify where the builder can find the vertices and indices for triangles, and their formats:
    VkAccelerationStructureGeometryTrianglesDataKHR triangles = nvvk::make<VkAccelerationStructureGeometryTrianglesDataKHR>();
    triangles.vertexFormat                                    = VK_FORMAT_R32G32B32_SFLOAT;
    triangles.vertexData.deviceAddress                        = vertexBufferAddress;
    triangles.vertexStride                                    = 3 * sizeof(float);
    triangles.maxVertex                                       = static_cast<uint32_t>(objVertices.size() - 1);
    triangles.indexType                                       = VK_INDEX_TYPE_UINT32;
    triangles.indexData.deviceAddress                         = indexBufferAddress;
    triangles.transformData.deviceAddress                     = 0;  // No transform
    // Create a VkAccelerationStructureGeometryKHR object that says it handles opaque triangles and points to the above:
    VkAccelerationStructureGeometryKHR geometry = nvvk::make<VkAccelerationStructureGeometryKHR>();
    geometry.geometry.triangles                 = triangles;
    geometry.geometryType                       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_KHR;
    blas.asGeometry.push_back(geometry);
    // Create offset info that allows us to say how many triangles and vertices to read
    VkAccelerationStructureBuildRangeInfoKHR offsetInfo;
    offsetInfo.firstVertex     = 0;
    offsetInfo.primitiveCount  = static_cast<uint32_t>(objIndices.size() / 3);  // Number of triangles
    offsetInfo.primitiveOffset = 0;
    offsetInfo.transformOffset = 0;
    blas.asBuildOffsetInfo.push_back(offsetInfo);
    blases.push_back(blas);
  }
  // Create the BLAS
  nvvk::RaytracingBuilderKHR raytracingBuilder;
  raytracingBuilder.setup(context, &allocator, context.m_queueGCT);
  raytracingBuilder.buildBlas(blases, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
                                          | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);

  // Create 441 instances with random rotations pointing to BLAS 0, and build these instances into a TLAS:
  std::vector<nvvk::RaytracingBuilderKHR::Instance> instances;
  std::default_random_engine                        randomEngine;  // The random number generator
  std::uniform_real_distribution<float>             uniformDist(-0.5f, 0.5f);
  std::uniform_int_distribution<int>                uniformIntDist(0, 8);
  for(int x = -10; x <= 10; x++)
  {
    for(int y = -10; y <= 10; y++)
    {
      nvvk::RaytracingBuilderKHR::Instance instance;
      instance.transform.translate(nvmath::vec3f(float(x), float(y), 0.0f));
      instance.transform.scale(1.0f / 2.7f);
      instance.transform.rotate(uniformDist(randomEngine), nvmath::vec3f(0.0f, 1.0f, 0.0f));
      instance.transform.rotate(uniformDist(randomEngine), nvmath::vec3f(1.0f, 0.0f, 0.0f));
      instance.transform.translate(nvmath::vec3f(0.0f, -1.0f, 0.0f));

      instance.instanceCustomId = 0;  // 24 bits accessible to ray shaders via gl_InstanceCustomIndex
      instance.blasId           = 0;  // The index of the BLAS in `blases` that this instance points to
      instance.hitGroupId = uniformIntDist(randomEngine);  // An offset that will be added when looking up the instance's shader in the SBT.
      instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;  // How to trace this instance
      instances.push_back(instance);
    }
  }
  raytracingBuilder.buildTlas(instances, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

  // Here's the list of bindings for the descriptor set layout, from raytrace.comp.glsl:
  // 0 - a storage image (the image `image`)
  // 1 - an acceleration structure (the TLAS)
  // 2 - a storage buffer (the vertex buffer)
  // 3 - a storage buffer (the index buffer)
  nvvk::DescriptorSetContainer descriptorSetContainer(context);
  descriptorSetContainer.addBinding(BINDING_IMAGEDATA, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  descriptorSetContainer.addBinding(BINDING_TLAS, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  descriptorSetContainer.addBinding(BINDING_VERTICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  descriptorSetContainer.addBinding(BINDING_INDICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  // Create a layout from the list of bindings
  descriptorSetContainer.initLayout();
  // Create a descriptor pool from the list of bindings with space for 1 set, and allocate that set
  descriptorSetContainer.initPool(1);
  // Create a push constant range describing the amount of data for the push constants.
  static_assert(sizeof(PushConstants) % 4 == 0, "Push constant size must be a multiple of 4 per the Vulkan spec!");
  VkPushConstantRange pushConstantRange;
  pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = sizeof(PushConstants);
  // Create a pipeline layout from the descriptor set layout and push constant range:
  descriptorSetContainer.initPipeLayout(1,                    // Number of push constant ranges
                                        &pushConstantRange);  // Pointer to push constant ranges

  // Write values into the descriptor set.
  std::array<VkWriteDescriptorSet, 4> writeDescriptorSets;
  // Color image
  VkDescriptorImageInfo descriptorImageInfo{};
  descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;  // The image's layout
  descriptorImageInfo.imageView   = imageView;                // How the image should be accessed
  writeDescriptorSets[0] = descriptorSetContainer.makeWrite(0 /*set index*/, BINDING_IMAGEDATA /*binding*/, &descriptorImageInfo);
  // Top-level acceleration structure (TLAS)
  VkWriteDescriptorSetAccelerationStructureKHR descriptorAS = nvvk::make<VkWriteDescriptorSetAccelerationStructureKHR>();
  VkAccelerationStructureKHR tlasCopy = raytracingBuilder.getAccelerationStructure();  // So that we can take its address
  descriptorAS.accelerationStructureCount = 1;
  descriptorAS.pAccelerationStructures    = &tlasCopy;
  writeDescriptorSets[1]                  = descriptorSetContainer.makeWrite(0, BINDING_TLAS, &descriptorAS);
  // Vertex buffer
  VkDescriptorBufferInfo vertexDescriptorBufferInfo{};
  vertexDescriptorBufferInfo.buffer = vertexBuffer.buffer;
  vertexDescriptorBufferInfo.range  = VK_WHOLE_SIZE;
  writeDescriptorSets[2] = descriptorSetContainer.makeWrite(0, BINDING_VERTICES, &vertexDescriptorBufferInfo);
  // Index buffer
  VkDescriptorBufferInfo indexDescriptorBufferInfo{};
  indexDescriptorBufferInfo.buffer = indexBuffer.buffer;
  indexDescriptorBufferInfo.range  = VK_WHOLE_SIZE;
  writeDescriptorSets[3]           = descriptorSetContainer.makeWrite(0, BINDING_INDICES, &indexDescriptorBufferInfo);
  vkUpdateDescriptorSets(context,                                            // The context
                         static_cast<uint32_t>(writeDescriptorSets.size()),  // Number of VkWriteDescriptorSet objects
                         writeDescriptorSets.data(),                         // Pointer to VkWriteDescriptorSet objects
                         0, nullptr);  // An array of VkCopyDescriptorSet objects (unused)

  // Shader loading and pipeline creation
  const size_t                                      NUM_C_HIT_SHADERS = 9;
  std::array<VkShaderModule, 2 + NUM_C_HIT_SHADERS> modules;
  modules[0] = nvvk::createShaderModule(context, nvh::loadFile("shaders/raytrace.rgen.glsl.spv", true, searchPaths));
  debugUtil.setObjectName(modules[0], "Ray generation module (raytrace.rgen.glsl.spv)");
  modules[1] = nvvk::createShaderModule(context, nvh::loadFile("shaders/raytrace.rmiss.glsl.spv", true, searchPaths));
  debugUtil.setObjectName(modules[1], "Miss module (raytrace.rmiss.glsl.spv)");
  for(int closestHitShaderIdx = 0; closestHitShaderIdx < NUM_C_HIT_SHADERS; closestHitShaderIdx++)
  {
    const int         moduleIdx = 2 + closestHitShaderIdx;
    const std::string filename  = "shaders/material" + std::to_string(closestHitShaderIdx) + ".rchit.glsl.spv";
    modules[moduleIdx]          = nvvk::createShaderModule(context, nvh::loadFile(filename, true, searchPaths));

    const std::string debugName = "Material " + std::to_string(closestHitShaderIdx) + " shader module";
    debugUtil.setObjectName(modules[moduleIdx], debugName);
  }

  // Create the shader binding table and ray tracing pipeline.
  // We'll create the ray tracing pipeline by specifying the shaders + layout,
  // and then get the handles of the shaders for the shader binding table from
  // the pipeline.
  VkPipeline            rtPipeline;
  nvvk::BufferDedicated rtSBTBuffer;  // The buffer for the Shader Binding Table
  {
    // First, we create objects that point to each of our shaders.
    // These are called "shader stages" in this context.
    // These are shader module + entry point + stage combinations, because each
    // shader module can contain multiple entry points (e.g. main1, main2...)
    std::array<VkPipelineShaderStageCreateInfo, 2 + NUM_C_HIT_SHADERS> stages;  // Pointers to shaders

    // Stage 0 will be the raygen shader.
    stages[0]        = nvvk::make<VkPipelineShaderStageCreateInfo>();
    stages[0].stage  = VK_SHADER_STAGE_RAYGEN_BIT_KHR;  // Kind of shader
    stages[0].module = modules[0];                      // Contains the shader
    stages[0].pName  = "main";                          // Name of the entry point
    // Stage 1 will be the miss shader.
    stages[1]        = stages[0];
    stages[1].stage  = VK_SHADER_STAGE_MISS_BIT_KHR;  // Kind of shader
    stages[1].module = modules[1];                    // Contains the shader
    // Stages 2 through the end will be closest-hit shaders.
    for(int closestHitShaderIdx = 0; closestHitShaderIdx < NUM_C_HIT_SHADERS; closestHitShaderIdx++)
    {
      const int moduleIdx      = 2 + closestHitShaderIdx;
      stages[moduleIdx]        = stages[0];
      stages[moduleIdx].stage  = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
      stages[moduleIdx].module = modules[moduleIdx];
    }

    // Then we make groups point to the shader stages. Each group can point to
    // 1-3 shader stages depending on the type, by specifying the index in the
    // stages array. These groups of handles then become the most important
    // part of the entries in the shader binding table.
    // Stores the indices of stages in each group:
    std::array<VkRayTracingShaderGroupCreateInfoKHR, 2 + NUM_C_HIT_SHADERS> groups;

    // The vkCmdTraceRays call will eventually refer to ray gen, miss, hit, and
    // callable shader binding tables and ranges.
    // A VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR group type is for a group
    // of one shader (a ray gen shader in a ray gen SBT region, a miss shader in
    // a miss SBT region, and so on.)
    // A VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR group type
    // is for an instance containing triangles. It can point to closest hit and
    // any hit shaders.
    // A VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR group type
    // is for a procedural instance, and can point to an intersection, any hit,
    // and closest hit shader.

    // We lay out our shader binding table like this:
    // RAY GEN REGION
    // Group 0 - points to Stage 0
    groups[0]               = nvvk::make<VkRayTracingShaderGroupCreateInfoKHR>();
    groups[0].type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[0].generalShader = 0;  // Index of ray gen, miss, or callable in `stages`
    // MISS SHADER REGION
    // Group 1 - points to Stage 1
    groups[1]               = nvvk::make<VkRayTracingShaderGroupCreateInfoKHR>();
    groups[1].type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[1].generalShader = 1;  // Index of ray gen, miss, or callable in `stages`
    // CLOSEST-HIT REGION
    // Group N - uses Stage N as its closest-hit shader
    for(int closestHitShaderIdx = 0; closestHitShaderIdx < NUM_C_HIT_SHADERS; closestHitShaderIdx++)
    {
      const int moduleIdx                = 2 + closestHitShaderIdx;
      groups[moduleIdx]                  = nvvk::make<VkRayTracingShaderGroupCreateInfoKHR>();
      groups[moduleIdx].type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
      groups[moduleIdx].closestHitShader = moduleIdx;  // Index of closest-hit in `stages`
    }

    // Now, describe the ray tracing pipeline, ike creating a compute pipeline:
    VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = nvvk::make<VkRayTracingPipelineCreateInfoKHR>();
    pipelineCreateInfo.flags                             = 0;  // No flags to set
    pipelineCreateInfo.stageCount                        = static_cast<uint32_t>(stages.size());
    pipelineCreateInfo.pStages                           = stages.data();
    pipelineCreateInfo.groupCount                        = static_cast<uint32_t>(groups.size());
    pipelineCreateInfo.pGroups                           = groups.data();
    pipelineCreateInfo.maxPipelineRayRecursionDepth      = 1;  // Depth of call tree
    pipelineCreateInfo.layout                            = descriptorSetContainer.getPipeLayout();
    NVVK_CHECK(vkCreateRayTracingPipelinesKHR(context,                 // Device
                                              VK_NULL_HANDLE,          // Deferred operation or VK_NULL_HANDLE
                                              VK_NULL_HANDLE,          // Pipeline cache or VK_NULL_HANDLE
                                              1, &pipelineCreateInfo,  // Array of create infos
                                              nullptr,                 // Allocator
                                              &rtPipeline));
    debugUtil.setObjectName(rtPipeline, "rtPipeline");

    // Now create and write the shader binding table, by getting the shader
    // group handles from the ray tracing pipeline and writing them into a
    // Vulkan buffer object.

    // Get the shader group handles:
    std::vector<uint8_t> cpuShaderHandleStorage(sbtHeaderSize * groups.size());
    NVVK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(context,                               // Device
                                                    rtPipeline,                            // Pipeline
                                                    0,                                     // First group
                                                    static_cast<uint32_t>(groups.size()),  // Number of groups
                                                    cpuShaderHandleStorage.size(),         // Size of buffer
                                                    cpuShaderHandleStorage.data()));       // Data buffer
    // Allocate the shader binding table. We get its device address, and
    // use it as a shader binding table. As before, we set its memory property
    // flags so that it can be read and written from the CPU.
    const uint32_t sbtSize = static_cast<uint32_t>(sbtStride * groups.size());
    rtSBTBuffer            = allocator.createBuffer(
        sbtSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    debugUtil.setObjectName(rtSBTBuffer.buffer, "rtSBTBuffer");
    // Copy the shader group handles to the SBT:
    uint8_t* mappedSBT = reinterpret_cast<uint8_t*>(allocator.map(rtSBTBuffer));
    for(size_t groupIndex = 0; groupIndex < groups.size(); groupIndex++)
    {
      memcpy(&mappedSBT[groupIndex * sbtStride], &cpuShaderHandleStorage[groupIndex * sbtHeaderSize], sbtHeaderSize);
    }
    allocator.unmap(rtSBTBuffer);
    // Clean up:
    allocator.finalizeAndReleaseStaging();
  }

  // vkCmdTraceRaysKHR uses VkStridedDeviceAddressregionKHR objects to say
  // where each block of shaders is held in memory. These could change per
  // draw call, but let's create them up front since they're the same
  // every time here:
  VkStridedDeviceAddressRegionKHR sbtRayGenRegion, sbtMissRegion, sbtHitRegion, sbtCallableRegion;
  const VkDeviceAddress           sbtStartAddress = GetBufferDeviceAddress(context, rtSBTBuffer.buffer);
  {
    // The ray generation shader region:
    sbtRayGenRegion.deviceAddress = sbtStartAddress;  // Starts here
    sbtRayGenRegion.stride        = sbtStride;        // Uses this stride
    sbtRayGenRegion.size          = sbtStride;        // Is this number of bytes long (1 group)

    sbtMissRegion               = sbtRayGenRegion;              // The miss shader region:
    sbtMissRegion.deviceAddress = sbtStartAddress + sbtStride;  // Starts sbtStride bytes (1 group) in
    sbtMissRegion.size          = sbtStride;                    // Is this number of bytes long (1 group)

    sbtHitRegion               = sbtRayGenRegion;                  // The hit group region:
    sbtHitRegion.deviceAddress = sbtStartAddress + 2 * sbtStride;  // Starts 2 * sbtStride bytes (2 groups) in
    sbtHitRegion.size          = sbtStride * NUM_C_HIT_SHADERS;    // Is this number of bytes long

    sbtCallableRegion      = sbtRayGenRegion;  // The callable shader region:
    sbtCallableRegion.size = 0;                // Is empty
  }

  const uint32_t NUM_SAMPLE_BATCHES = 32;
  for(uint32_t sampleBatch = 0; sampleBatch < NUM_SAMPLE_BATCHES; sampleBatch++)
  {
    // Create and start recording a command buffer
    VkCommandBuffer cmdBuffer = AllocateAndBeginOneTimeCommandBuffer(context, cmdPool);

    // Bind the ray tracing pipeline:
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline);
    // Bind the descriptor set
    VkDescriptorSet descriptorSet = descriptorSetContainer.getSet(0);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, descriptorSetContainer.getPipeLayout(),
                            0, 1, &descriptorSet, 0, nullptr);

    // Push push constants:
    pushConstants.sample_batch = sampleBatch;
    vkCmdPushConstants(cmdBuffer,                               // Command buffer
                       descriptorSetContainer.getPipeLayout(),  // Pipeline layout
                       VK_SHADER_STAGE_RAYGEN_BIT_KHR,          // Stage flags
                       0,                                       // Offset
                       sizeof(PushConstants),                   // Size in bytes
                       &pushConstants);                         // Data

    // Run the ray tracing pipeline and trace rays
    vkCmdTraceRaysKHR(cmdBuffer,           // Command buffer
                      &sbtRayGenRegion,    // Region of memory with ray generation groups
                      &sbtMissRegion,      // Region of memory with miss groups
                      &sbtHitRegion,       // Region of memory with hit groups
                      &sbtCallableRegion,  // Region of memory with callable groups
                      render_width,        // Width of dispatch
                      render_height,       // Height of dispatch
                      1);                  // Depth of dispatch

    // On the last sample batch:
    if(sampleBatch == NUM_SAMPLE_BATCHES - 1)
    {
      // Transition `image` from GENERAL to TRANSFER_SRC_OPTIMAL layout. See the
      // code for uploadCmdBuffer above to see a description of what this does:
      const VkAccessFlags        srcAccesses = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      const VkAccessFlags        dstAccesses = VK_ACCESS_TRANSFER_READ_BIT;
      const VkPipelineStageFlags srcStages   = nvvk::makeAccessMaskPipelineStageFlags(srcAccesses);
      const VkPipelineStageFlags dstStages   = nvvk::makeAccessMaskPipelineStageFlags(dstAccesses);
      const VkImageMemoryBarrier barrier =
          nvvk::makeImageMemoryBarrier(image.image,               // The VkImage
                                       srcAccesses, dstAccesses,  // Src and dst access masks
                                       VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  // Src and dst layouts
                                       VK_IMAGE_ASPECT_COLOR_BIT);
      vkCmdPipelineBarrier(cmdBuffer,             // Command buffer
                           srcStages, dstStages,  // Src and dst pipeline stages
                           0,                     // Dependency flags
                           0, nullptr,            // Global memory barriers
                           0, nullptr,            // Buffer memory barriers
                           1, &barrier);          // Image memory barriers

      // Now, copy the image (which has layout TRANSFER_SRC_OPTIMAL) to imageLinear
      // (which has layout TRANSFER_DST_OPTIMAL).
      {
        VkImageCopy region;
        // We copy the image aspect, layer 0, mip 0:
        region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount     = 1;
        region.srcSubresource.mipLevel       = 0;
        // (0, 0, 0) in the first image corresponds to (0, 0, 0) in the second image:
        region.srcOffset      = {0, 0, 0};
        region.dstSubresource = region.srcSubresource;
        region.dstOffset      = {0, 0, 0};
        // Copy the entire image:
        region.extent = {render_width, render_height, 1};
        vkCmdCopyImage(cmdBuffer,                             // Command buffer
                       image.image,                           // Source image
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  // Source image layout
                       imageLinear.image,                     // Destination image
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // Destination image layout
                       1, &region);                           // Regions
      }

      // Add a command that says "Make it so that memory writes by transfers
      // are available to read from the CPU." (In other words, "Flush the GPU caches
      // so the CPU can read the data.") To do this, we use a memory barrier.
      VkMemoryBarrier memoryBarrier = nvvk::make<VkMemoryBarrier>();
      memoryBarrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;  // Make transfer writes
      memoryBarrier.dstAccessMask   = VK_ACCESS_HOST_READ_BIT;       // Readable by the CPU
      vkCmdPipelineBarrier(cmdBuffer,                                // The command buffer
                           VK_PIPELINE_STAGE_TRANSFER_BIT,           // From transfers
                           VK_PIPELINE_STAGE_HOST_BIT,               // To the CPU
                           0,                                        // No special flags
                           1, &memoryBarrier,                        // An array of memory barriers
                           0, nullptr, 0, nullptr);                  // No other barriers
    }

    // End and submit the command buffer, then wait for it to finish:
    EndSubmitWaitAndFreeCommandBuffer(context, context.m_queueGCT, cmdPool, cmdBuffer);

    nvprintf("Rendered sample batch index %d.\n", sampleBatch);
  }

  // Get the image data back from the GPU
  void* data;
  NVVK_CHECK(vkMapMemory(context, imageLinear.allocation, 0, VK_WHOLE_SIZE, 0, &data));
  stbi_write_hdr("out.hdr", render_width, render_height, 4, reinterpret_cast<float*>(data));
  vkUnmapMemory(context, imageLinear.allocation);

  allocator.destroy(rtSBTBuffer);
  vkDestroyPipeline(context, rtPipeline, nullptr);
  for(VkShaderModule& shaderModule : modules)
  {
    vkDestroyShaderModule(context, shaderModule, nullptr);
  }
  descriptorSetContainer.deinit();
  raytracingBuilder.destroy();
  allocator.destroy(vertexBuffer);
  allocator.destroy(indexBuffer);
  vkDestroyCommandPool(context, cmdPool, nullptr);
  allocator.destroy(imageLinear);
  vkDestroyImageView(context, imageView, nullptr);
  allocator.destroy(image);
  allocator.deinit();
  context.deinit();  // Don't forget to clean up at the end of the program!
}
