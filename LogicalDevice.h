#pragma once

#include "GraphicsHeaders.h"
#include "PhysicalDevice.h"

class LogicalDevice
{
public:
	LogicalDevice(const PhysicalDevice*& physicalDevice, const uint32_t graphicsQueueIndex, const uint32_t presentQueueIndex, const std::vector<const char*>& requestedExtensions, const bool enableValidationLayers)
	{
		const float queuePriorities[] = { 1.0f };
		const VkDeviceQueueCreateInfo deviceGraphicsQueueCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = graphicsQueueIndex,
			.queueCount = 1,
			.pQueuePriorities = queuePriorities
		};

		const VkDeviceQueueCreateInfo devicePresentQueueCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = presentQueueIndex,
			.queueCount = 1,
			.pQueuePriorities = queuePriorities
		};

		VkDeviceQueueCreateInfo deviceQueueCreateInfos[] = { deviceGraphicsQueueCreateInfo, devicePresentQueueCreateInfo };

		VkPhysicalDeviceFeatures physicalDeviceFeatures = {};

		VkDeviceCreateInfo deviceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = deviceQueueCreateInfos,
			.enabledExtensionCount = static_cast<uint32_t>(requestedExtensions.size()),
			.ppEnabledExtensionNames = &requestedExtensions[0],
			.pEnabledFeatures = &physicalDeviceFeatures
		};

		if (enableValidationLayers) {
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(NUM_VALIDATION_LAYERS);
			deviceCreateInfo.ppEnabledLayerNames = &VALIDATION_LAYERS[0];
		}
		else {
			deviceCreateInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice->GetHandle(), &deviceCreateInfo, nullptr, &logicalDeviceHandle_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical Vulkan device for selected GPU.");
		}

		vkGetDeviceQueue(logicalDeviceHandle_, graphicsQueueIndex, 0, &graphicsQueue_);
		vkGetDeviceQueue(logicalDeviceHandle_, presentQueueIndex, 0, &presentQueue_);
	}

	const VkDevice& GetHandle() const
	{
		return logicalDeviceHandle_;
	}
private:
	VkDevice logicalDeviceHandle_;
	VkQueue graphicsQueue_;
	VkQueue presentQueue_;
	
	inline static const int NUM_VALIDATION_LAYERS = 1;
	inline static const char* VALIDATION_LAYERS[NUM_VALIDATION_LAYERS] = {
		"VK_LAYER_KHRONOS_validation"
	};
};
