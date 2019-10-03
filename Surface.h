#pragma once

#include "GraphicsHeaders.h"

#include <vector>
#include "SwapChain.h"
#include "PhysicalDevice.h"

class Surface
{
public:
	Surface(const VkSurfaceKHR& surfaceHandle, const PhysicalDevice*& physicalDevice)
		: surfaceHandle_(surfaceHandle)
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice->GetHandle(), surfaceHandle_, &surfaceCapabilities_);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->GetHandle(), surfaceHandle_, &formatCount, nullptr);

		if (formatCount != 0) {
			surfaceFormats_.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->GetHandle(), surfaceHandle_, &formatCount, &surfaceFormats_[0]);
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice->GetHandle(), surfaceHandle_, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			presentModes_.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice->GetHandle(), surfaceHandle_, &presentModeCount, &presentModes_[0]);
		}
	}

	const VkSurfaceFormatKHR& QuerySurfaceFormat(const VkFormat format = VK_FORMAT_B8G8R8A8_UNORM, const VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) const
	{
		const auto foundFormat = std::find_if(surfaceFormats_.begin(), surfaceFormats_.end(), [](auto fmt) -> bool { return fmt.format == format && fmt.colorSpace == colorSpace; });
		if (foundFormat != surfaceFormats_.end()) {
			return *foundFormat;
		}
		return surfaceFormats_[0];
	}

	VkPresentModeKHR GetPresentModeOrDefault(const VkPresentModeKHR& presentMode = VK_PRESENT_MODE_MAILBOX_KHR) const
	{
		if (std::find(presentModes_.begin(), presentModes_.end(), presentMode) != presentModes_.end()) {
			return presentMode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	void ClampExtentToSurface(VkExtent2D& extent) const
	{
		if (surfaceCapabilities_.currentExtent.width != UINT32_MAX) {
			extent = surfaceCapabilities_.currentExtent;
		}
		else
		{
			extent = {
				std::clamp(static_cast<uint32_t>(extent.width), surfaceCapabilities_.minImageExtent.width,
						   surfaceCapabilities_.maxImageExtent.width),
				std::clamp(static_cast<uint32_t>(extent.height), surfaceCapabilities_.minImageExtent.height,
						   surfaceCapabilities_.maxImageExtent.height)
			};
		}
	}
private:
	const VkSurfaceKHR& surfaceHandle_;
	VkSurfaceCapabilitiesKHR surfaceCapabilities_;
	std::vector<VkSurfaceFormatKHR> surfaceFormats_;
	std::vector<VkPresentModeKHR> presentModes_;
};
