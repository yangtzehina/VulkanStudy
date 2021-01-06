# Install script for directory: G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/../_install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/shared_sources/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/_edit/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/1_hello_vulkan/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/2_extensions/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/3_memory/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/4_commands/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/5_write_image/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/6_compute_shader/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/7_descriptors/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/8_ray_tracing/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/9_intersection/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/10_mesh_data/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/11_specular/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/12_antialiasing/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/vk_mini_path_tracer/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e1_gaussian/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e3_compaction/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e4_include/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e5_push_constants/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e6_more_samples/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e7_image/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e8_debug_names/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e9_instances/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e10_materials/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e11_rt_pipeline_1/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e11_rt_pipeline_2/cmake_install.cmake")
  include("G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/checkpoints/e11_rt_pipeline_3/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "G:/UnityGraphics/vulkan/VulkanStudy/vk_mini_path_tracer/cmake_build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
