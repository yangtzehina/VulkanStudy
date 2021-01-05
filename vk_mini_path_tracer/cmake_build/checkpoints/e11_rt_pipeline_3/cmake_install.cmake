# Install script for directory: G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "G:/UnityGraphics/vulkan/vk_mini_path_tracer/../_install")
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

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin_x64" TYPE EXECUTABLE FILES "G:/UnityGraphics/vulkan/bin_x64/Release/vk_mini_path_tracer_e11_rt_pipeline_3.exe")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin_x64_debug" TYPE EXECUTABLE FILES "G:/UnityGraphics/vulkan/bin_x64/Debug/vk_mini_path_tracer_e11_rt_pipeline_3.exe")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin_x64/vk_mini_path_tracer_e11_rt_pipeline_3/shaders" TYPE FILE FILES
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material0.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material1.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material2.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material3.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material4.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material5.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material6.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material7.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material8.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/raytrace.rgen.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/raytrace.rmiss.glsl.spv"
      )
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin_x64_debug/vk_mini_path_tracer_e11_rt_pipeline_3/shaders" TYPE FILE FILES
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material0.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material1.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material2.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material3.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material4.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material5.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material6.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material7.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/material8.rchit.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/raytrace.rgen.glsl.spv"
      "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/shaders/raytrace.rmiss.glsl.spv"
      )
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin_x64/vk_mini_path_tracer_e11_rt_pipeline_3" TYPE DIRECTORY FILES "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/../../scenes")
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin_x64_debug/vk_mini_path_tracer_e11_rt_pipeline_3" TYPE DIRECTORY FILES "G:/UnityGraphics/vulkan/vk_mini_path_tracer/checkpoints/e11_rt_pipeline_3/../../scenes")
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

