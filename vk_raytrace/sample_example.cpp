/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <sstream>
#include <vulkan/vulkan.hpp>

#include "nvh/cameramanipulator.hpp"
#include "nvh/fileoperations.hpp"
#include "nvh/nvprint.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/shaders_vk.hpp"

#include "binding.h"
#include "imgui_helper.h"
#include "rtx_pipeline.hpp"
#include "sample_example.hpp"
#include "tools.hpp"


#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "fileformats/tiny_gltf.h"
#include "fileformats/tiny_gltf_freeimage.h"

#include "imgui_orient.h"
#include "nvml_monitor.hpp"

static NvmlMonitor g_nvml(100, 100);

//--------------------------------------------------------------------------------------------------
// Keep the handle on the device
// Initialize the tool to do all our allocations: buffers, images
//
void SampleExample::setup(const vk::Instance&       instance,
                          const vk::Device&         device,
                          const vk::PhysicalDevice& physicalDevice,
                          uint32_t                  gtcQueueIndex,
                          uint32_t                  computeQueueIndex)
{
  AppBase::setup(instance, device, physicalDevice, gtcQueueIndex);

  m_memAllocator.init(device, physicalDevice);
  m_memAllocator.setAllocateFlags(VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, true);
  m_alloc.init(device, physicalDevice, &m_memAllocator);
  m_debug.setup(m_device);

  m_sunAndSky = SunAndSky_default();
  m_picker.setup(m_device, physicalDevice, computeQueueIndex, &m_alloc);
  m_scene.setup(m_device, physicalDevice, gtcQueueIndex, &m_alloc);
  m_accelStruct.setup(m_device, physicalDevice, computeQueueIndex, &m_alloc);
  m_offscreen.setup(m_device, physicalDevice, gtcQueueIndex, &m_alloc);
  m_skydome.setup(device, physicalDevice, gtcQueueIndex, &m_alloc);

  // Create and setup all renderers
  m_pRender[eRtxPipeline] = new RtxPipeline;

  for(auto r : m_pRender)
    r->setup(m_device, physicalDevice, gtcQueueIndex, &m_alloc);
}


//--------------------------------------------------------------------------------------------------
// Loading the scene file and setting up all buffers
//
void SampleExample::loadScene(const std::string& filename)
{
  m_scene.load(filename);
  m_accelStruct.create(m_scene.getScene(), m_scene.getBuffer(Scene::eVertex).buffer, m_scene.getBuffer(Scene::eIndex).buffer);

  m_picker.initialize(m_accelStruct.getTlas());
}

//--------------------------------------------------------------------------------------------------
//
//
void SampleExample::loadEnvironmentHdr(const std::string& hdrFilename)
{
  MilliTimer timer;
  LOGI("Loading HDR and converting %s\n", hdrFilename.c_str());
  m_skydome.loadEnvironment(hdrFilename);
  LOGI(" --> (%5.3f ms)\n", timer.elapse());

  //m_offscreen.m_tReinhard.avgLum = m_skydome.getAverage();
  m_state.fireflyClampThreshold = m_skydome.getIntegral() * 4.f;  //magic
}


//--------------------------------------------------------------------------------------------------
// Loading asset in a separate thread
// - Used by file drop and menu operation
// Marking the session as busy, to avoid calling rendering while loading assets
//
void SampleExample::loadAssets(const char* filename)
{
  std::string sfile = filename;

  // Need to stop current rendering
  m_busy = true;
  m_device.waitIdle();

  std::thread([&, sfile]() {
    LOGI("Loading: %s\n", sfile.c_str());

    // Supporting only GLTF and HDR files
    namespace fs          = std::filesystem;
    std::string extension = fs::path(sfile).extension().string();
    if(extension == ".gltf" || extension == ".glb")
    {
      m_busyReasonText = "Loading scene ";

      // Loading scene and creating acceleration structure
      loadScene(sfile);
      // Loading the scene might have loaded new textures, which is changing the number of elements
      // in the DescriptorSetLayout. Therefore, the PipelineLayout will be out-of-date and need
      // to be re-created. If they are re-created, the pipeline also need to be re-created.
      m_pRender[m_rndMethod]->create(
          m_size, {m_accelStruct.getDescLayout(), m_offscreen.getDescLayout(), m_scene.getDescLayout(), m_descSetLayout}, &m_scene);
    }

    if(extension == ".hdr")  //|| extension == ".exr")
    {
      m_busyReasonText = "Loading HDR ";
      loadEnvironmentHdr(sfile);
      updateHdrDescriptors();
    }


    // Re-starting the frame count to 0
    SampleExample::resetFrame();
    m_busy = false;
  }).detach();
}


//--------------------------------------------------------------------------------------------------
// Called at each frame to update the environment (sun&sky)
//
void SampleExample::updateUniformBuffer(const vk::CommandBuffer& cmdBuf)
{
  m_scene.updateCamera(cmdBuf);
  cmdBuf.updateBuffer<SunAndSky>(m_sunAndSkyBuffer.buffer, 0, m_sunAndSky);
}

//--------------------------------------------------------------------------------------------------
// If the camera matrix has changed, resets the frame otherwise, increments frame.
//
void SampleExample::updateFrame()
{
  static nvmath::mat4f refCamMatrix;
  static float         fov = 0;

  auto& m = CameraManip.getMatrix();
  auto  f = CameraManip.getFov();
  if(memcmp(&refCamMatrix.a00, &m.a00, sizeof(nvmath::mat4f)) != 0 || f != fov)
  {
    resetFrame();
    refCamMatrix = m;
    fov          = f;
  }

  if(m_state.frame < m_maxFrames)
    m_state.frame++;
}

void SampleExample::resetFrame()
{
  m_state.frame = -1;
}

//--------------------------------------------------------------------------------------------------
// Descriptors for the Sun&Sky buffer
//
void SampleExample::createDescriptorSetLayout()
{
  using vkDS = vk::DescriptorSetLayoutBinding;
  using vkDT = vk::DescriptorType;
  using vkSS = vk::ShaderStageFlagBits;

  auto flag = vkSS::eClosestHitKHR | vkSS::eAnyHitKHR | vkSS::eCompute | vkSS::eFragment;

  m_bind.addBinding(vkDS(B_SUNANDSKY, vkDT::eUniformBuffer, 1, vkSS::eMissKHR | flag));
  m_bind.addBinding(vkDS(B_HDR, vkDT::eCombinedImageSampler, 1, vkSS::eClosestHitKHR | vkSS::eMissKHR));  // HDR image
  m_bind.addBinding(vkDS(B_IMPORT_SMPL, vkDT::eCombinedImageSampler, 1, vkSS::eClosestHitKHR));  // importance sampling


  m_descSetLayout = m_bind.createLayout(m_device);
  m_descPool      = m_bind.createPool(m_device, 1);
  m_descSet       = nvvk::allocateDescriptorSet(m_device, m_descPool, m_descSetLayout);
  m_debug.setObjectName(m_descSetLayout, "env");
  m_debug.setObjectName(m_descSet, "env");

  // Using the environment
  std::vector<vk::WriteDescriptorSet> writes;
  vk::DescriptorBufferInfo            sunskyDesc{m_sunAndSkyBuffer.buffer, 0, VK_WHOLE_SIZE};
  writes.emplace_back(m_bind.makeWrite(m_descSet, B_SUNANDSKY, &sunskyDesc));
  writes.emplace_back(m_bind.makeWrite(m_descSet, B_HDR, &m_skydome.m_textures.txtHdr.descriptor));
  writes.emplace_back(m_bind.makeWrite(m_descSet, B_IMPORT_SMPL, &m_skydome.m_textures.accelImpSmpl.descriptor));

  m_device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void SampleExample::updateHdrDescriptors()
{
  std::vector<vk::WriteDescriptorSet> writes;
  writes.emplace_back(m_bind.makeWrite(m_descSet, B_HDR, &m_skydome.m_textures.txtHdr.descriptor));
  writes.emplace_back(m_bind.makeWrite(m_descSet, B_IMPORT_SMPL, &m_skydome.m_textures.accelImpSmpl.descriptor));
  m_device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Creating the uniform buffer holding the camera matrices
// - Buffer is host visible
//
void SampleExample::createUniformBuffer()
{
  m_sunAndSkyBuffer = m_alloc.createBuffer(sizeof(SunAndSky),
                                           vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
                                           vk::MemoryPropertyFlagBits::eDeviceLocal);
  m_debug.setObjectName(m_sunAndSkyBuffer.buffer, "sunsky");
}

//--------------------------------------------------------------------------------------------------
// Destroying all allocations
//
void SampleExample::destroyResources()
{
  // Resources
  m_alloc.destroy(m_sunAndSkyBuffer);

  // Descriptors
  m_device.destroy(m_descPool);
  m_device.destroy(m_descSetLayout);

  // Other
  m_picker.destroy();
  m_scene.destroy();
  m_accelStruct.destroy();
  m_offscreen.destroy();
  m_skydome.destroy();
  m_axis.deinit();

  // All renderers
  for(auto p : m_pRender)
  {
    p->destroy();
    p = nullptr;
  }

  // Memory
  m_alloc.deinit();
  m_memAllocator.deinit();
}

//--------------------------------------------------------------------------------------------------
// Handling resize of the window
//
void SampleExample::onResize(int /*w*/, int /*h*/)
{
  m_offscreen.update(m_size);
  if(m_rndMethod != eRtxPipeline)  // RtxPipeline doesn't care about size, only on run
    m_pRender[m_rndMethod]->create(
        m_size, {m_accelStruct.getDescLayout(), m_offscreen.getDescLayout(), m_scene.getDescLayout(), m_descSetLayout}, &m_scene);
  resetFrame();
}

//////////////////////////////////////////////////////////////////////////
// Post-processing
//////////////////////////////////////////////////////////////////////////

void SampleExample::createOffscreenRender()
{
  m_offscreen.create(m_size, m_renderPass);
  m_axis.init(m_device, m_renderPass);
}


void SampleExample::drawPost(vk::CommandBuffer cmdBuf)
{
  m_offscreen.m_tReinhard.zoom = m_descaling ? 1.0f / m_descalingLevel : 1.0f;
  m_offscreen.run(cmdBuf, m_size);
  if(m_showAxis)
    m_axis.display(cmdBuf, CameraManip.getMatrix(), m_size);
}

//////////////////////////////////////////////////////////////////////////
// Ray tracing / Wavefront
//////////////////////////////////////////////////////////////////////////

void SampleExample::render(RndMethod method, const vk::CommandBuffer& cmdBuf, nvvk::ProfilerVK& profiler)
{
  g_nvml.refresh();

  // We are done rendering
  if(m_state.frame >= m_maxFrames)
    return;

  if(method != m_rndMethod)
  {
    LOGI("Switching renderer, from %d to %d \n", m_rndMethod, method);
    m_device.waitIdle();  // cannot destroy while in use
    if(m_rndMethod != eNone)
      m_pRender[m_rndMethod]->destroy();
    m_rndMethod = method;

    m_pRender[m_rndMethod]->create(
        m_size, {m_accelStruct.getDescLayout(), m_offscreen.getDescLayout(), m_scene.getDescLayout(), m_descSetLayout}, &m_scene);
  }

  vk::Extent2D render_size = m_size;
  if(m_descaling)
    render_size = vk::Extent2D(m_size.width / m_descalingLevel, m_size.height / m_descalingLevel);

  m_pRender[m_rndMethod]->m_state = m_state;
  m_pRender[m_rndMethod]->run(cmdBuf, render_size, profiler,
                              {m_accelStruct.getDescSet(), m_offscreen.getDescSet(), m_scene.getDescSet(), m_descSet});
}


//////////////////////////////////////////////////////////////////////////
/// GUI
//////////////////////////////////////////////////////////////////////////
using GuiH = ImGuiH::Control;


//--------------------------------------------------------------------------------------------------
//
//
void SampleExample::titleBar()
{
  static float dirtyTimer = 0.0f;

  dirtyTimer += ImGui::GetIO().DeltaTime;
  if(dirtyTimer > 1)
  {
    std::stringstream o;
    o << "VK glTF Viewer";
    o << " | " << m_scene.getSceneName();                     // Scene name
    o << " | " << m_size.width << "x" << m_size.height;       // resolution
    o << " | " << static_cast<int>(ImGui::GetIO().Framerate)  // FPS / ms
      << " FPS / " << std::setprecision(3) << 1000.F / ImGui::GetIO().Framerate << "ms";
    if(g_nvml.isValid())  // Graphic card, driver
    {
      const auto& i = g_nvml.getInfo(0);
      o << " | " << i.name;
      o << " | " << g_nvml.getSysInfo().driverVersion;
    }
    glfwSetWindowTitle(m_window, o.str().c_str());
    dirtyTimer = 0;
  }
}

void SampleExample::menuBar()
{
  auto openFilename = [](const char* filter) {
#ifdef _WIN32
    char         filename[MAX_PATH];
    OPENFILENAME ofn;
    ZeroMemory(&filename, sizeof(filename));
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = nullptr;  // If you have a window to center over, put its HANDLE here
    ofn.lpstrFilter = filter;
    ofn.lpstrFile   = filename;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = "Select a File";
    ofn.Flags       = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    if(GetOpenFileNameA(&ofn))
    {
      return std::string(filename);
    }
#endif

    return std::string("");
  };


  // Menu Bar
  if(ImGui::BeginMainMenuBar())
  {
    if(ImGui::BeginMenu("File"))
    {
      if(ImGui::MenuItem("Open GLTF Scene"))
        loadAssets(openFilename("GLTF Files\0*.gltf\0Binary\0*.glb\0").c_str());
      if(ImGui::MenuItem("Open HDR Environment"))
        loadAssets(openFilename("HDR Files\0*.hdr\0\0").c_str());
      ImGui::Separator();
      if(ImGui::MenuItem("Quit", "ESC"))
        glfwSetWindowShouldClose(m_window, 1);
      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Tools"))
    {
      ImGui::MenuItem("Settings", "F10", &m_show_gui);
      ImGui::MenuItem("Axis", nullptr, &m_showAxis);
      ImGui::EndMenu();
    }
    ImGuiH::Panel::style.panel_offset.y = ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }
}

//--------------------------------------------------------------------------------------------------
//
//
bool SampleExample::guiCamera()
{
  bool changed{false};
  changed |= ImGuiH::CameraWidget();
  GuiH::Checkbox("Show Axis", "Show orientation axis on the lower left corner", &m_showAxis);
  return changed;
}

bool SampleExample::guiRayTracing()
{
  auto Normal = ImGuiH::Control::Flags::Normal;
  bool changed{false};

  changed |= GuiH::Slider("Max Ray Depth", "", &m_state.maxDepth, nullptr, Normal, 1, 10);
  changed |= GuiH::Slider("Samples Per Frame", "", &m_state.maxSamples, nullptr, Normal, 1, 10);
  changed |= GuiH::Slider("Max Iteration ", "", &m_maxFrames, nullptr, Normal, 1, 1000);
  changed |= GuiH::Selection("Debug Mode", "Display unique values of material", &m_state.debugging_mode, nullptr, Normal,
                             {"No Debug", "BaseColor", "Normal", "Metallic", "Emissive", "Alpha", "Roughness",
                              "Textcoord", "Tangent", "Radiance", "Weight", "RayDir"});

  changed |= GuiH::Slider("De-scaling ",
                          "Reduce resolution while navigating.\n"
                          "Speeding up rendering while camera moves.\n"
                          "Value of 1, will not de-scale",
                          &m_descalingLevel, nullptr, Normal, 1, 8);

  GuiH::Info("Frame", "", std::to_string(m_state.frame), GuiH::Flags::Disabled);
  return changed;
}


bool SampleExample::guiTonemapper()
{
  static Offscreen::TReinhard default_tm;  // default values
  auto&                       tm = m_offscreen.m_tReinhard;
  bool                        changed{false};

  // changed |= GuiH::Slider("Brightness", "Brightness adjustment of the output image", &tm.key, &default_tm.key,
  //                        GuiH::Flags::Normal, 0.00f, 2.00f);
  // changed |= GuiH::Slider("White Burning", "More or less white", &tm.Ywhite, &default_tm.Ywhite, GuiH::Flags::Normal, 0.001f, 10.00f);
  // changed |= GuiH::Slider("Saturation", "Color to grey", &tm.sat, &default_tm.sat, GuiH::Flags::Normal, 0.00f, 2.00f);

  // changed |= GuiH::Slider("Luminance", "Average luminance of the scene", &tm.avgLum, &default_tm.avgLum,
  //                        GuiH::Flags::Normal, 0.001f, 25.00f);

  changed |= GuiH::Slider("Exposure", "Scene Exposure", &tm.avgLum, &default_tm.avgLum, GuiH::Flags::Normal, 0.001f, 5.00f);

  return false;  // doesn't matter
}

bool SampleExample::guiEnvironment()
{
  static SunAndSky dss = SunAndSky_default();  // default values
  bool             changed{false};

  changed |= ImGui::Checkbox("Use Sun & Sky", (bool*)&m_sunAndSky.in_use);
  changed |= GuiH::Slider("Exposure", "Intensity of the environment", &m_state.environment_intensity_factor, nullptr,
                          GuiH::Flags::Normal, 0.f, 5.f);

  if(m_sunAndSky.in_use)
  {
    GuiH::Group<bool>("Sun", true, [&] {
      changed |= GuiH::Custom("Direction", "Sun Direction", [&] {
        float indent = ImGui::GetCursorPos().x;
        changed |= ImGui::DirectionGizmo("", &m_sunAndSky.sun_direction.x, true);
        ImGui::NewLine();
        ImGui::SameLine(indent);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        changed |= ImGui::InputFloat3("##IG", &m_sunAndSky.sun_direction.x);
        return changed;
      });
      changed |= GuiH::Slider("Disk Scale", "", &m_sunAndSky.sun_disk_scale, &dss.sun_disk_scale, GuiH::Flags::Normal, 0.f, 100.f);
      changed |= GuiH::Slider("Glow Intensity", "", &m_sunAndSky.sun_glow_intensity, &dss.sun_glow_intensity,
                              GuiH::Flags::Normal, 0.f, 5.f);
      changed |= GuiH::Slider("Disk Intensity", "", &m_sunAndSky.sun_disk_intensity, &dss.sun_disk_intensity,
                              GuiH::Flags::Normal, 0.f, 5.f);
      changed |= GuiH::Color("Night Color", "", &m_sunAndSky.night_color.x, &dss.night_color.x, GuiH::Flags::Normal);
      return changed;
    });

    GuiH::Group<bool>("Ground", true, [&] {
      changed |=
          GuiH::Slider("Horizon Height", "", &m_sunAndSky.horizon_height, &dss.horizon_height, GuiH::Flags::Normal, -1.f, 1.f);
      changed |= GuiH::Slider("Horizon Blur", "", &m_sunAndSky.horizon_blur, &dss.horizon_blur, GuiH::Flags::Normal, 0.f, 1.f);
      changed |= GuiH::Color("Ground Color", "", &m_sunAndSky.ground_color.x, &dss.ground_color.x, GuiH::Flags::Normal);
      changed |= GuiH::Slider("Haze", "", &m_sunAndSky.haze, &dss.haze, GuiH::Flags::Normal, 0.f, 15.f);
      return changed;
    });

    GuiH::Group<bool>("Other", false, [&] {
      changed |= GuiH::Drag("Multiplier", "", &m_sunAndSky.multiplier, &dss.multiplier, GuiH::Flags::Normal, 0.f,
                            std::numeric_limits<float>::max(), 2, "%5.5f");
      changed |= GuiH::Slider("Saturation", "", &m_sunAndSky.saturation, &dss.saturation, GuiH::Flags::Normal, 0.f, 1.f);
      changed |= GuiH::Slider("Red Blue Shift", "", &m_sunAndSky.redblueshift, &dss.redblueshift, GuiH::Flags::Normal, -1.f, 1.f);
      changed |= GuiH::Color("RGB Conversion", "", &m_sunAndSky.rgb_unit_conversion.x, &dss.rgb_unit_conversion.x,
                             GuiH::Flags::Normal);

      nvmath::vec3f eye, center, up;
      CameraManip.getLookat(eye, center, up);
      m_sunAndSky.y_is_up = up.y == 1;
      changed |= GuiH::Checkbox("Y is Up", "", (bool*)&m_sunAndSky.y_is_up, nullptr, GuiH::Flags::Disabled);
      return changed;
    });
  }

  return changed;
}

bool SampleExample::guiStatistics()
{
  ImGuiStyle& style    = ImGui::GetStyle();
  auto        pushItem = style.ItemSpacing;
  style.ItemSpacing.y  = -4;  // making the lines more dense

  auto& stats = m_scene.getStat();

  if(stats.nbCameras > 0)
    GuiH::Info("Cameras", "", FormatNumbers(stats.nbCameras));
  if(stats.nbImages > 0)
    GuiH::Info("Images", "", FormatNumbers(stats.nbImages) + " (" + FormatNumbers(stats.imageMem) + ")");
  if(stats.nbTextures > 0)
    GuiH::Info("Textures", "", FormatNumbers(stats.nbTextures));
  if(stats.nbMaterials > 0)
    GuiH::Info("Material", "", FormatNumbers(stats.nbMaterials));
  if(stats.nbSamplers > 0)
    GuiH::Info("Samplers", "", FormatNumbers(stats.nbSamplers));
  if(stats.nbNodes > 0)
    GuiH::Info("Nodes", "", FormatNumbers(stats.nbNodes));
  if(stats.nbMeshes > 0)
    GuiH::Info("Meshes", "", FormatNumbers(stats.nbMeshes));
  if(stats.nbLights > 0)
    GuiH::Info("Lights", "", FormatNumbers(stats.nbLights));
  if(stats.nbTriangles > 0)
    GuiH::Info("Triangles", "", FormatNumbers(stats.nbTriangles));
  if(stats.nbUniqueTriangles > 0)
    GuiH::Info("Unique Tri", "", FormatNumbers(stats.nbUniqueTriangles));
  GuiH::Info("Resolution", "", std::to_string(m_size.width) + "x" + std::to_string(m_size.height));

  style.ItemSpacing = pushItem;

  return false;
}

bool SampleExample::guiProfiler(nvvk::ProfilerVK& profiler)
{
  static int m_avg = 10;
  //  static double m_statsCpuTime, m_statsGpuTime = 0;
  static vec2 statRender;
  static vec2 statTone;
  static vec2 statUi;

  int avgf = profiler.getTotalFrames();
  if(avgf % m_avg == m_avg - 1)
  {
    nvh::Profiler::TimerInfo info;
    profiler.getTimerInfo("Render", info);
    statRender.x = float(info.gpu.average);
    statRender.y = float(info.cpu.average);
    profiler.getTimerInfo("Tonemap", info);
    statTone.x = float(info.gpu.average);
    statTone.y = float(info.cpu.average);
    profiler.getTimerInfo("Ui", info);
    statUi.x = float(info.gpu.average);
    statUi.y = float(info.cpu.average);

    m_avg = std::max(static_cast<int>(ImGui::GetIO().Framerate / 10.F), 1);  // 10 lookup per second
  }

  float maxTimeF  = std::max(std::max(statRender.y, statRender.x), 0.0001f);
  float frameTime = 1000.0f / ImGui::GetIO().Framerate;
  ImGui::Text("Frame     [ms]: %2.3f", frameTime);
  ImGui::Text("Render GPU/CPU [ms]: %2.3f  /  %2.3f", statRender.x / 1000.0f, statRender.y / 1000.0f);
  //  ImGuiH::Text("Scene CPU [ms]: %2.3f", cpuTimeF / 1000.0f);
  ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x / 2);
  ImGui::ProgressBar(statRender.x / maxTimeF, ImVec2(0.0f, 0.0f));
  ImGui::SameLine(0, 0);
  ImGui::ProgressBar(statRender.y / maxTimeF, ImVec2(0.0f, 0.0f));
  ImGui::PopItemWidth();
  ImGui::Text("Tone+UI GPU/CPU [ms]: %2.3f  /  %2.3f", statTone.x / 1000.0f, statTone.y / 1000.0f);


  std::vector<std::string> sections = {"Generate", "Extend", "Shade", "Connect", "Finalize"};
  std::array<float, 5>     values;
  nvh::Profiler::TimerInfo info;
  for(size_t i = 0; i < sections.size(); i++)
  {
    profiler.getTimerInfo(sections[i].c_str(), info);
    values[i] = static_cast<float>(info.gpu.average);
  }


  // Display histogram only if there is a valid value
  if(values[0] > 0)
  {
    ImGui::PlotHistogram("##histo", values.data(), static_cast<int>(values.size()), 0, nullptr, 0, maxTimeF,
                         ImVec2(ImGui::GetContentRegionAvail().x, 90.f));
    std::string s = std::accumulate(sections.begin(), sections.end(), std::string(),
                                    [](const std::string& a, const std::string& b) -> std::string {
                                      return a + (a.length() > 0 ? " | " : "") + b;
                                    });
    ImGui::Text(s.c_str());
  }


  return false;
}


bool SampleExample::guiGpuMeasures()
{
  if(g_nvml.isValid() == false)
    ImGui::Text("NVML wasn't loaded");

  auto memoryNumbers = [](float n, int precision = 3) -> std::string {
    std::vector<std::string> t{" KB", " MB", " GB", " TB"};
    int                      level{0};
    while(n > 1024)
    {
      n = n / 1024;
      level++;
    }
    assert(level < 3);
    std::stringstream o;
    o << std::setprecision(precision) << std::fixed << n << t[level];

    return o.str();
  };

  uint32_t offset = g_nvml.getOffset();

  for(uint32_t g = 0; g < g_nvml.nbGpu(); g++)  // Number of gpu
  {
    const auto& i = g_nvml.getInfo(g);
    const auto& m = g_nvml.getMeasures(g);

    std::stringstream o;
    o << "Driver: " << i.driver_model << "\n"                                                                //
      << "Memory: " << memoryNumbers(m.memory[offset]) << "/" << memoryNumbers(float(i.max_mem), 0) << "\n"  //
      << "Load: " << m.load[offset];

    float mem = m.memory[offset] / float(i.max_mem) * 100.f;
    char  desc[64];
    sprintf(desc, "%s: \n- Load: %2.0f%s \n- Mem: %2.0f%s", i.name.c_str(), m.load[offset], "%%", mem, "%%");
    ImGuiH::Control::Custom(desc, o.str().c_str(), [&]() {
      ImGui::ImPlotMulti datas[2];
      datas[0].plot_type     = ImGuiPlotType_Area;
      datas[0].name          = "Load";
      datas[0].color         = ImColor(0.07f, 0.9f, 0.06f, 1.0f);
      datas[0].thickness     = 1.5;
      datas[0].data          = m.load.data();
      datas[0].values_count  = (int)m.load.size();
      datas[0].values_offset = offset + 1;
      datas[0].scale_min     = 0;
      datas[0].scale_max     = 100;

      datas[1].plot_type     = ImGuiPlotType_Histogram;
      datas[1].name          = "Mem";
      datas[1].color         = ImColor(0.06f, 0.6f, 0.97f, 0.8f);
      datas[1].thickness     = 2.0;
      datas[1].data          = m.memory.data();
      datas[1].values_count  = (int)m.memory.size();
      datas[1].values_offset = offset + 1;
      datas[1].scale_min     = 0;
      datas[1].scale_max     = float(i.max_mem);


      std::string overlay = std::to_string((int)m.load[offset]) + " %";
      ImGui::PlotMultiEx("##NoName", 2, datas, overlay.c_str(), ImVec2(0, 100));

      return false;
    });

    ImGuiH::Control::Custom("CPU", "", [&]() {
      ImGui::ImPlotMulti datas[1];
      datas[0].plot_type     = ImGuiPlotType_Lines;
      datas[0].name          = "CPU";
      datas[0].color         = ImColor(0.96f, 0.96f, 0.07f, 1.0f);
      datas[0].thickness     = 1.0;
      datas[0].data          = g_nvml.getSysInfo().cpu.data();
      datas[0].values_count  = (int)g_nvml.getSysInfo().cpu.size();
      datas[0].values_offset = offset + 1;
      datas[0].scale_min     = 0;
      datas[0].scale_max     = 100;

      std::string overlay = std::to_string((int)m.load[offset]) + " %";
      ImGui::PlotMultiEx("##NoName", 1, datas, nullptr, ImVec2(0, 0));

      return false;
    });
  }


  return false;
}


//--------------------------------------------------------------------------------------------------
// Display a static window when loading assets
//
void SampleExample::showBusyWindow()
{
  static int   nb_dots   = 0;
  static float deltaTime = 0;
  bool         show      = true;
  size_t       width     = 270;
  size_t       height    = 60;

  deltaTime += ImGui::GetIO().DeltaTime;
  if(deltaTime > .25)
  {
    deltaTime = 0;
    nb_dots   = ++nb_dots % 10;
  }

  ImGui::SetNextWindowSize(ImVec2(float(width), float(height)));
  ImGui::SetNextWindowPos(ImVec2(float(m_size.width - width) * 0.5f, float(m_size.height - height) * 0.5f));

  ImGui::SetNextWindowBgAlpha(0.75f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 15.0);
  if(ImGui::Begin("##notitle", &show,
                  ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
                      | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs))
  {
    ImVec2 available = ImGui::GetContentRegionAvail();

    ImVec2 text_size = ImGui::CalcTextSize(m_busyReasonText.c_str(), nullptr, false, available.x);

    ImVec2 pos = ImGui::GetCursorPos();
    pos.x += (available.x - text_size.x) * 0.5f;
    pos.y += (available.y - text_size.y) * 0.5f;

    ImGui::SetCursorPos(pos);
    ImGui::TextWrapped((m_busyReasonText + std::string(nb_dots, '.')).c_str());
  }
  ImGui::PopStyleVar();
  ImGui::End();
}


//////////////////////////////////////////////////////////////////////////
// Keyboard / Drag and Drop
//////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Overload keyboard hit
// - Home key: fit all, the camera will move to see the entire scene bounding box
// - Space: Trigger ray picking and set the interest point at the intersection
//          also return all information under the cursor
//
void SampleExample::onKeyboard(int key, int scancode, int action, int mods)
{
  nvvk::AppBase::onKeyboard(key, scancode, action, mods);

  if(action == GLFW_RELEASE)
    return;

  if(key == GLFW_KEY_HOME)
  {
    // Set the camera as to see the model
    fitCamera(m_scene.getScene().m_dimensions.min, m_scene.getScene().m_dimensions.max, false);
  }
  else if(key == GLFW_KEY_SPACE)
  {
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);

    // Set the camera as to see the model
    nvvk::CommandPool sc(m_device, m_graphicsQueueIndex);
    vk::CommandBuffer cmdBuf = sc.createCommandBuffer();

    const float aspectRatio = m_size.width / static_cast<float>(m_size.height);
    auto        view        = CameraManip.getMatrix();
    auto        proj        = nvmath::perspectiveVK(CameraManip.getFov(), aspectRatio, 0.1f, 1000.0f);

    RayPickerKHR::PickInfo pickInfo;
    pickInfo.pickX          = float(x) / float(m_size.width);
    pickInfo.pickY          = float(y) / float(m_size.height);
    pickInfo.modelViewInv   = nvmath::invert(view);
    pickInfo.perspectiveInv = nvmath::invert(proj);


    m_picker.run(cmdBuf, pickInfo);
    sc.submitAndWait(cmdBuf);

    RayPickerKHR::PickResult pr = m_picker.getResult();

    if(pr.instanceID == ~0)
    {
      LOGI("Not Hit\n");
      return;
    }

    nvmath::vec3 worldPos = pr.worldRayOrigin + pr.worldRayDirection * pr.hitT;
    // Set the interest position
    nvmath::vec3f eye, center, up;
    CameraManip.getLookat(eye, center, up);
    CameraManip.setLookat(eye, worldPos, up, false);
  }
  else if(key == GLFW_KEY_R)
  {
    resetFrame();
  }
}

void SampleExample::onFileDrop(const char* filename)
{
  loadAssets(filename);
}


//--------------------------------------------------------------------------------------------------
// Window callback when the mouse move
// - Handling ImGui and a default camera
//
void SampleExample::onMouseMotion(int x, int y)
{
  AppBase::onMouseMotion(x, y);

  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard)
    return;

  if(m_inputs.lmb || m_inputs.rmb || m_inputs.mmb)
  {
    m_descaling = true;
  }
}

void SampleExample::onMouseButton(int button, int action, int mods)
{
  AppBase::onMouseButton(button, action, mods);
  if((m_inputs.lmb || m_inputs.rmb || m_inputs.mmb) == false && action == GLFW_RELEASE && m_descaling == true)
  {
    m_descaling = false;
    resetFrame();
  }
}
