#pragma once

#include "GraphicsHeaders.h"

#include <stdexcept>
#include <string>
#include <vector>

class Instance
{
public:
	Instance(const std::string& applicationName, const uint32_t applicationVersion, const std::string& engineName, const uint32_t engineVersion, const uint32_t enabledExtensionCount, const char **enabledExtensions, const bool enableValidationLayers, const std::vector<const char*>& validationLayers)
	{
		VkApplicationInfo applicationInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = applicationName.c_str(),
			.applicationVersion = applicationVersion,
			.pEngineName = engineName.c_str(),
			.engineVersion = engineVersion,
			.apiVersion = VK_API_VERSION_1_1
		};

		VkInstanceCreateInfo instanceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &applicationInfo,
			.enabledLayerCount = 0,
			.enabledExtensionCount = enabledExtensionCount,
			.ppEnabledExtensionNames = enabledExtensions
		};

		if (enableValidationLayers && IsValidationLayerSupported(validationLayers)) {
			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = &validationLayers[0];
		}

		if (vkCreateInstance(&instanceCreateInfo, nullptr, &instanceHandle_) != VK_SUCCESS) {
			throw std::runtime_error("Could not create Vulkan instance.");
		}

		// Enumerate physical devices
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instanceHandle_, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("Could not find any available GPUs with Vulkan support.");
		}

		std::vector<VkPhysicalDevice> availableDevices(deviceCount);
		vkEnumeratePhysicalDevices(instanceHandle_, &deviceCount, &availableDevices[0]);
		std::vector<PhysicalDevice*> physicalDevices(deviceCount);
		std::transform(availableDevices.begin(), availableDevices.end(), physicalDevices.begin(), [](auto handle) -> PhysicalDevice* { return new PhysicalDevice(handle); });
	}

	~Instance()
	{
		vkDestroyInstance(instanceHandle_, nullptr);
	}

	const VkInstance& GetHandle() const
	{
		return instanceHandle_;
	}
private:
	VkInstance instanceHandle_;
	const std::vector<PhysicalDevice*> physicalDevices_;

	static bool IsValidationLayerSupported(const std::vector<const char*>& validationLayers) {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, &availableLayers[0]);

		for (auto layerName : validationLayers) {
			auto layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}
};
