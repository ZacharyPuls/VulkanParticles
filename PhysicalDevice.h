#pragma once

#include "GraphicsHeaders.h"

#include <stdexcept>
#include <vector>
#include <string>
#include <set>

class PhysicalDevice
{
public:
	PhysicalDevice(const VkPhysicalDevice& physicalDeviceHandle) : physicalDeviceHandle_(physicalDeviceHandle)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle_, &queueFamilyCount, nullptr);

		queueFamilies_.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle_, &queueFamilyCount, &queueFamilies_[0]);
	}

	~PhysicalDevice()
	{

	}

	bool SupportsExtensions(const std::vector<const char *>& extensions) const
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDeviceHandle_, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDeviceHandle_, nullptr, &extensionCount, &availableExtensions[0]);

		std::set<std::string> requestedExtensions(extensions.begin(), extensions.end());

		for (const auto& extension : availableExtensions) {
			requestedExtensions.erase(extension.extensionName);
		}

		return requestedExtensions.empty();
	}

	VkPhysicalDeviceMemoryProperties GetMemoryProperties() const
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDeviceHandle_, &memoryProperties);
		return memoryProperties;
	}

	uint32_t FindMemoryType(const uint32_t& memoryTypeBits, const VkMemoryPropertyFlags& memoryPropertyFlags) const
	{
		auto memoryProperties = GetMemoryProperties();
		auto suitableMemoryType = UINT32_MAX;
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
		{
			if ((memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
			{
				suitableMemoryType = i;
			}
		}

		if (suitableMemoryType == UINT32_MAX)
		{
			throw std::runtime_error("Failed to find suitable memory type for Buffer.");
		}

		return suitableMemoryType;
	}
	VkPhysicalDevice GetHandle() const
	{
		return physicalDeviceHandle_;
	}

private:
	const VkPhysicalDevice& physicalDeviceHandle_;
	std::vector<VkQueueFamilyProperties> queueFamilies_;
};
