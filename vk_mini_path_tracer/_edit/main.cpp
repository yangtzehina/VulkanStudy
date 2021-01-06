// Copyright 2020 NVIDIA Corporation
// SPDX-License-Identifier: Apache-2.0
#include <cassert>

#include <nvvk/context_vk.hpp>
#include <nvvk/structs_vk.hpp>

int main(int argc, const char** argv)
{
	// Create the Vulkan context, consisting of an instance, device, physical device, and queues.
	nvvk::ContextCreateInfo deviceInfo;  // One can modify this to load different extensions or pick the Vulkan core version
	deviceInfo.apiMajor = 1;
	deviceInfo.apiMinor = 2; //这里是设置使用的Vulkan版本，apiMajor是开头第一个数，比如1.2.0
	deviceInfo.addDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	nvvk::Context           context;     // Encapsulates device state in a single object
	context.init(deviceInfo);            // Initialize the context
	//vkAllocateCommandBuffers(context, nullptr, nullptr); //Invalid call!!
	context.deinit();                    // Don't forget to clean up at the end of the program!
}