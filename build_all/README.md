![logo](http://nvidianews.nvidia.com/_ir/219/20157/NV_Designworks_logo_horizontal_greenblack.png)

# nvpro-samples

The `build_all` folder is an optional folder that allows you to synchronize and build all the samples that you have cloned using a single solution.

* CMakeLists.txt: the cmake file that will walk through samples to include them in the project
* README.md: this file
* LICENSE.md: the license used for all nvpro-samples
* batch/script files: allows to easily clone/pull all existing samples

Running `clone_all` batch/script will create following directory structure

```bash
build_all
shared_sources
shared_external
... (all repositories specified in the script)
```

Each sample can be built either individually, or with `build_all/CMakeLists.txt` as single solution. You can still configure the solution for `build_all` to only include a subset of projects with the appropriate `BUILD_sample_name` checkbox in the cmake-ui.

All samples must be built for a 64-bit architecture, and most samples require C++14. All of these samples support Windows (MSVC 2017 is our minimum compiler), while many of them also support Linux as well (more comprehensive Linux support is in progress).

# Shared Dependencies

* [shared_sources](https://github.com/nvpro-samples/shared_sources): The primary framework that all samples depend on. Contains window management, ui, and various api helpers.
* [shared_external](https://github.com/nvpro-samples/shared_external): Third party libraries that are provided pre-compiled, mostly for Windows x64 / MSVC.


# Vulkan Samples

These samples are "pure" Vulkan samples and use its WSI system to create the window swapchain.

## [vk_async_resources](https://github.com/nvpro-samples/vk_async_resources)

![screenshot-vk_async_resources](doc/vk_async_resources.png)

In Vulkan lifetime management such as deleting resources is a bit more complex than in OpenGL.
The basic sample describes a strategy that delays deletion of Vulkan resources for a few frames.
Furthermore Vulkan provides multiple ways to upload data to the device, three different approaches
are described.

## [vk_device_generated_cmds](https://github.com/nvpro-samples/vk_device_generated_cmds)

![screenshot-vk_device_generated_cmds](doc/vk_device_generated_cmds.png)

In this sample the functionality of the VK_NV_device_generated_commands is demonstrated. This extension greatly enhanes the indirect drawing capabilities and adds the ability to change shaders on the device.
Furthermore the usage of bindless buffers is shown, as an alternative to the classic descriptor set binding model.

* Loads `.csf` and `.gltf 2` models
* VK_NV_device_generated_commands
* VK_EXT_buffer_device_address
* GLSL_EXT_buffer_reference

## [vk_shaded_gltfscene](https://github.com/nvpro-samples/vk_shaded_gltfscene)

![screenshot-vk_shaded_gltfscene](doc/vk_shaded_gltfscene.png)

Load a [glTF](https://www.khronos.org/gltf/) scene with materials and textures. Display a HDR image in the background and use it for lighting the scene. It renders in multiple passes, background, scene, then tonemap the result and add UI at the end. Shows how to deal with many objects, many materials and textures. This example will push the material parameters through `push_constant` and uses different descriptor sets to enable the textures to use. It also shows how to read the depth buffer to un-project the mouse coordinate to 3D position to set the camera interest.

* Loads `.gltf 2` models

**Tags**: GLTF, PBR material, HDR, tonemapper, textures, mipmapping, debugging shader, depth buffer reading, unproject, importance sampling, cubemap

## [vk_raytrace](https://github.com/nvpro-samples/vk_raytrace)

![screenshot-vk_raytrace](doc/vk_raytrace.png)

Reads a [glTF](https://www.khronos.org/gltf/) scene and renders the scene using NVIDIA ray tracing. It uses techniques like image base lighting and importance sampling, reflections, transparency and indirect illumination. The camera simulates a pin-hole whitted camera and the image is toned mapped using various tone mappers.

The example shows as well how to implement a picking ray, which is using the same acceleration structure for drawing, but is using the hit data to return the information under the mouse cursor. This information can be use for setting the camera interest position, or to debug any shading data.

* Loads `.gltf 2` models
* VK_NV_ray_tracing
* VK_EXT_descriptor_indexing

**Tags**: raytracing, GLTF, HDR, tonemapper, picking, BLAS, TLAS, PBR material

## [vk_denoise](https://github.com/nvpro-samples/vk_denoise)

![screenshot-vk_denoise](doc/vk_denoise.png)

This example is an extension of the vk_raytrace example. After a few iterations, the image will be denoised using the [Optix7 denoiser](https://developer.nvidia.com/optix-denoiser). To achieve this, an interop between CUDA and Vulkan is set. Vulkan images are converted to CUDA buffers and converted back after been denoised. This pass is inserted between other rendering passes, as it is done in vk_raytrace.

* Loads `.gltf 2` models
* VK_NV_ray_tracing
* VK_EXT_descriptor_indexing
* VK_KHR_external_memory
* VK_KHR_external_semaphore
* VK_KHR_external_fence

**Tags**: ray tracing, path tracing, GLTF, HDR, tonemapper, picking, BLAS, TLAS, PBR material, denoising, Cuda, interop, OptiX

## [vk_mini_path_tracer](https://github.com/nvpro-samples/vk_mini_path_tracer)

![screenshot-vk_mini_path_tracer](doc/vk_mini_path_tracer.png)

A beginner-friendly Vulkan path tracing tutorial in under 300 lines of C++. Intended as both an introduction to Vulkan, and as an introduction to computer graphics through ray tracing. Includes tips and tricks along the way, and extra chapters show how to extend the path tracer, implement production techniques, and use a performance analysis tool. Dovetails into vk_raytracing_tutorial_KHR.

* VK_KHR_acceleration_structure
* VK_KHR_shader_non_semantic_info
* VK_KHR_ray_query
* VK_KHR_ray_tracing_pipeline

**Tags**: ray tracing, path tracing, ray queries, ray tracing pipelines, compute shaders, debug printf, BLAS, TLAS, OBJ, beginner

## [vk_raytracing_tutorial_KHR](https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR)

![screenshot-vk_tutorial](doc/vk_raytracing_tutorial.png)

A tutorial that explains step-by-step what is needed to add ray tracing to an existing Vulkan application. The first tutorial is the base of ray tracing, and from this base, many other tutorials are explaining the various features of RTX. 

* Explain Vulkan ray tracing
* Animating BLAS and TLAS
* Using any hit shaders
* Using memory managers for handling many objects and instances
* Using an intersection shader and rendering implicit geometries
* Jittering camera ray generation and image accumulation for anti-aliased images
* Using various closest hit shaders
* Using shader record to modify the behavior of the shader.
* Recursive reflection vs iterative reflection

**Tags**: ray tracing, OBJ, tonemapper, BLAS, TLAS

## [vk_offline](https://github.com/nvpro-samples/vk_offline)

![screenshot-vk_offline](doc/vk_offline.png)

Simple offline application which using Vulkan to render without opening a window.

* Very simple Vulkan offline rendering
* Create Vulkan context
* Render to frame buffer
* Save frame buffer to disk (PNG)

**Tags**: compute shader, offline rendering

## [vk_order_independent_transparency](https://github.com/nvpro-samples/vk_order_independent_transparency)

![screenshot-vk_order_independent_transparency](doc/vk_order_independent_transparency.png)

Demonstates seven different techniques for rendering transparent objects without requiring them to be sorted in advance.

* Shows seven different ways to implement transparency
* Includes antialiasing techniques and linear colorspace rendering
* Render pass subpasses used to implement Weighted, Blended Order-Independent Transparency
* Shows how to construct linked lists on the GPU
* Includes example of fragment shader interlock (`GL_ARB_fragment_shader_interlock`, much like rasterizer order views in Direct3D 11.3)
* Shows how to use 64-bit atomics and the `VK_KHR_shader_atomic_int64` extension.

**Tags**: transparency, subpasses, MSAA, algorithms

## [vk_toon_shader](https://github.com/nvpro-samples/vk_toon_shader)

![screenshot-vk_toon](doc/vk_toon_shader.png)

Rendering object outlines and details from canvases render with rasterizer or ray tracer.

* Extracting object contours
* Rendering lines for normal and depth discontinuities
* Post-process chaining, image processed used by next post-process
* FXAA on line buffers
* Toon effect with shading and [Kuwahara](https://en.wikipedia.org/wiki/Kuwahara_filter) post-effect

**Tags**: silhouette, contour, toon shading, post-process, fxaa

## [glsl_indexed_types_generator](https://github.com/nvpro-samples/glsl_indexed_types_generator)

This project serves as proof of concept how to simplify the usage of `VK_EXT_descriptor_indexing` 
and `GL_EXT_nonuniform_qualifier` within GLSL (typically used in combination with `VK_NV_ray_tracing`).
A Lua script generates structures and function overloads to hide the code for indexing descriptor
sets of samplers and textures.

* stand-alone, does not depend on shared_sources
* VK_EXT_descriptor_indexing
* GL_EXT_nonuniform_qualifier

# OpenGL / Vulkan Samples

These samples use the `gl_vk_` prefix and showcase Vulkan and OpenGL techniques within
the same application (`gl_vk_sample_name.exe`) or just Vulkan alone (`vk_sample_name.exe`). If available, using the `BUILD_gl_vk_sample_name_VULKAN_ONLY` option, you can omit building the combined executable file. The `VULKAN_ONLY` mode uses Vulkan's WSI system to create the swapchain, the combined executable uses `GL_NV_draw_vulkan_image`.

## [gl_vk_threaded_cadscene](https://github.com/nvpro-samples/gl_vk_meshlet_cadscene)

![screenshot-gl_vk_threaded_cadscene](doc/gl_vk_threaded_cadscene.png)

OpenGL and Vulkan comparison on rendering a CAD scene using various techniques. Stresses
CPU bottlenecks due to the scene having lots of tiny drawcalls. Also touches upon different ways how to provide per-draw data in Vulkan, as well as how to create drawcalls on multiple threads in both OpenGL and Vulkan.

* Loads `.csf` and `.gltf 2` models
* GL_NV_draw_vulkan_image (not used in `VULKAN_ONLY`)
* GL_NV_command_list
* GL_NV_vertex_buffer_unified_memory
* GL_NV_uniform_buffer_unified_memory

## [gl_vk_meshlet_cadscene](https://github.com/nvpro-samples/gl_vk_threaded_cadscene)

![screenshot-gl_vk_meshlet_cadscene](doc/gl_vk_meshlet_cadscene.png)

This OpenGL/Vulkan sample illustrates the use of [mesh shaders](https://devblogs.nvidia.com/introduction-turing-mesh-shaders/) for rendering CAD models.

* Loads `.csf` and `.gltf 2` models
* GL_NV_draw_vulkan_image (not used in `VULKAN_ONLY`)
* GL_NV_mesh_shader
* VK_NV_mesh_shader

## [gl_vk_chopper](https://github.com/nvpro-samples/gl_vk_chopper)

![screenshot-gl_vk_chopper](doc/gl_vk_chopper.png)

Renders an articulated scene with animated and textured models.

* GL_NV_draw_vulkan_image

## [gl_vk_bk3dthreaded](https://github.com/nvpro-samples/gl_vk_bk3dthreaded)

![screenshot-gl_vk_bk3dthreaded](doc/gl_vk_bk3dthreaded.jpg)

Vulkan sample rendering 3D with worker-threads

* GL_NV_draw_vulkan_image

## [gl_vk_supersampled](https://github.com/nvpro-samples/gl_vk_supersampled)

![screenshot-gl_vk_supersampled](doc/gl_vk_supersampled.jpg)

Vulkan sample showing a high quality super-sampled rendering

* GL_NV_draw_vulkan_image


## [gl_vk_simple_interop](https://github.com/nvpro-samples/gl_vk_simple_interop)

![screenshot-gl_vk_simple_interop](doc/gl_vk_simple_interop.png)

Rendering an animated image using a Vulkan compute shader and displaying this image
using OpenGL on an animated triangle. The image is allocated with Vulkan and shared
using Interop.

* GL_EXT_memory_object
* GL_EXT_semaphore
* VK_KHR_external_memory
* VK_KHR_external_semaphore
* VK_KHR_external_fence

## [gl_vk_raytrace_interop](https://github.com/nvpro-samples/gl_vk_raytrace_interop)

![screenshot-gl_vk_raytrace_interop](doc/gl_vk_raytrace_interop.png)

This example is adding ray traced ambient occlusion in an OpenGL scene.
All buffers are shared between OpenGL and Vulkan to create the acceleration
structure needed to ray trace. Rays are sent from the G-Buffer position rendered
by the OpenGL rasterizer.

* GL_EXT_memory_object
* GL_EXT_semaphore
* VK_NV_ray_tracing
* VK_KHR_external_memory
* VK_KHR_external_semaphore
* VK_KHR_external_fence

## [gl_render_vk_direct_display](https://github.com/nvpro-samples/gl_render_vk_direct_display)

![screenshot-gl_render_vk_direct_display](doc/gl_render_vk_direct_display.png)

This example shows how to use Vulkan Direct Display functionality from an OpenGL renderer.
A Vulkan Direct Display class provides render textures to an OpenGL renderer, which after
rendering submits the textures back to the Vulkan class for presentation on the
Direct Display device.

* VK_KHR_display
* VK_KHR_external_memory
* VK_KHR_external_semaphore

# OpenGL Samples

## [gl_cadscene_rendertechniques](https://github.com/nvpro-samples/gl_cadscene_rendertechniques)

![screenshot-gl_cadscene_rendertechniques](doc/gl_cadscene_rendertechniques.jpg)

OpenGL sample on various rendering approaches for typical CAD scenes. Stresses
CPU bottlenecks due to lots of low-complexity drawcalls.

* Loads `.csf` and `.gltf 2` models
* GL_ARB_multi_draw_indirect
* GL_NV_command_list
* GL_NV_vertex_buffer_unified_memory
* GL_NV_uniform_buffer_unified_memory

## [gl_occlusion_culling](https://github.com/nvpro-samples/gl_occlusion_culling)

![screenshot-gl_occlusion_culling](doc/gl_occlusion_culling.png)

Sample for shader-based occlusion culling, which is more scalable on modern GPUs
than traditional occlusion query techniques. Also showcases how to generate
drawcalls on the GPU, so that occlusion culling techniques don't need CPU
readbacks.

* GL_ARB_multi_draw_indirect
* GL_ARB_indirect_parameters
* GL_NV_command_list
* GL_NV_representative_fragment_test

## [gl_ssao](https://github.com/nvpro-samples/gl_ssao)

![screenshot-gl_ssao](doc/gl_ssao.jpg)

Optimized screen-space ambient occlusion, cache-aware HBAO

* GL_NV_geometry_shader_passthrough

## [gl_dynamic_lod](https://github.com/nvpro-samples/gl_dynamic_lod)

![screenshot-gl_commandlist_basic](doc/gl_dynamic_lod.jpg)

GPU classifies how to render millions of particles. Close/large particles
use tessellation, medium sized particles use an optimized instancing technique and distant
particles are rendered as points. No CPU readbacks needed.

* GL_ARB_compute_shader
* GL_ARB_multi_draw_indirect

## [gl_commandlist_basic](https://github.com/nvpro-samples/gl_commandlist_basic)

![screenshot-gl_commandlist_basic](doc/gl_commandlist_basic.jpg)

Basic sample for NV_command_list

* GL_NV_command_list

## [gl_path_rendering_CMYK](https://github.com/nvpro-samples/gl_path_rendering_CMYK)

![screenshot-gl_path_rendering_cmyk](doc/gl_path_rendering_cmyk.jpg)

Example of how to use path rendering; and how to use it with CMYK (using multi-render target)

* GL_NV_path_rendering

## [gl_multicast](https://github.com/nvpro-samples/gl_multicast)

Basic sample showcasing multicast capabilities, where one GL stream is very
efficiently sent to multiple GPUs. Typical use-case is for example VR SLI, where
each GPU renders a different eye.

* GL_NV_gpu_multicast

# NVML Samples

## [nvml_enterprise_gpu_check](https://github.com/nvpro-samples/nvml_enterprise_gpu_check)

Shows how to correctly load the NVML library for GPU information, and to robustly check
using NVML's API if a GPU is an Enterprise/Quadro GPU. (This works even when the GPU,
such as the RTX A6000, doesn't have "Quadro" in its name.

* nvmlInit
* nvmlDeviceGetCount
* nvmlDeviceGetHandleByIndex
* nvmlDeviceGetBrand