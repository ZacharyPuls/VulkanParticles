#pragma once

#include "GraphicsHeaders.h"

#include <stdexcept>
#include <vector>
#include <string>
#include <set>
#include <optional>

class PhysicalDevice
{
public:
	struct QueueFamilyIndices {
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;

		bool IsComplete()
		{
			return GraphicsFamily.has_value() && PresentFamily.has_value();
		}
	};
	
	PhysicalDevice(const VkPhysicalDevice physicalDeviceHandle) : physicalDeviceHandle(physicalDeviceHandle)
	{
		
	}

	~PhysicalDevice()
	{
		
	}

	const VkPhysicalDevice& GetHandle() const
	{
		return physicalDeviceHandle;
	}

	bool SupportsExtensions(const std::vector<const char *>& extensions) const
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDeviceHandle, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDeviceHandle, nullptr, &extensionCount, &availableExtensions[0]);

		std::set<std::string> requestedExtensions(extensions.begin(), extensions.end());

		for (const auto& extension : availableExtensions) {
			requestedExtensions.erase(extension.extensionName);
		}

		return requestedExtensions.empty();
	}

	QueueFamilyIndices GetQueueFamilyIndices(const VkSurfaceKHR& surface) const
	{
		QueueFamilyIndices queueFamilyIndices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle, &queueFamilyCount, &queueFamilies[0]);

		for (int i = 0; i < queueFamilyCount; ++i) {
			if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				queueFamilyIndices.GraphicsFamily = i;
			}

			VkBool32 presentationSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDeviceHandle, i, surface, &presentationSupport);

			if (queueFamilies[i].queueCount > 0 && presentationSupport) {
				queueFamilyIndices.PresentFamily = i;
			}

			if (queueFamilyIndices.IsComplete()) {
				break;
			}
		}

		return queueFamilyIndices;
	}

	const VkPhysicalDeviceMemoryProperties GetMemoryProperties() const
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDeviceHandle, &memoryProperties);
		return memoryProperties;
	}

	static std::vector<PhysicalDevice*> Enumerate(const VkInstance& instance, const VkSurfaceKHR& surface)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("Could not find any available GPUs with Vulkan support.");
		}

		std::vector<VkPhysicalDevice> availableDevices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, &availableDevices[0]);
		std::vector<PhysicalDevice*> physicalDevices(deviceCount);
		std::transform(availableDevices.begin(), availableDevices.end(), physicalDevices.begin(), [](auto handle) -> PhysicalDevice* { return new PhysicalDevice(handle); });
		return physicalDevices;
	}
private:
	VkPhysicalDevice physicalDeviceHandle;
};
