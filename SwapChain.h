#pragma once

#include "GraphicsHeaders.h"
#include "PhysicalDevice.h"
#include "Framebuffer.h"

#include <stdexcept>
#include <vector>
#include <algorithm>

class SwapChain
{
public:
	struct SupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		static SupportDetails Query(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface)
		{
			SupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

			if (formatCount != 0) {
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, &details.formats[0]);
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0) {
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, &details.presentModes[0]);
			}

			return details;
		}

		const VkSurfaceFormatKHR SelectFormat() const
		{
			const auto format = std::find_if(formats.begin(), formats.end(), [](auto fmt) -> bool { return fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; });
			if (format != formats.end()) {
				return *format;
			}
			else {
				return formats[0];
			}
		}

		const VkPresentModeKHR SelectPresentMode() const
		{
			if (std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.end()) {
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
			
			return VK_PRESENT_MODE_FIFO_KHR;
		}

		const VkExtent2D SelectSwapExtent(GLFWwindow*& window) const
		{
			if (capabilities.currentExtent.width != UINT32_MAX) {
				return capabilities.currentExtent;
			}
			int width;
			int height;

			glfwGetFramebufferSize(window, &width, &height);

			return { std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
				std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
		}
	};
	
	SwapChain(const VkDevice& logicalDevice, PhysicalDevice*& physicalDevice, const VkSurfaceKHR& surface, GLFWwindow*& window) : logicalDevice(logicalDevice)
	{
		const auto swapChainSupport = SupportDetails::Query(physicalDevice->GetHandle(), surface);
		const auto surfaceFormat = swapChainSupport.SelectFormat();
		const auto presentMode = swapChainSupport.SelectPresentMode();
		const auto swapExtent = swapChainSupport.SelectSwapExtent(window);
		uint32_t imageCount = std::min(swapChainSupport.capabilities.minImageCount + 1, swapChainSupport.capabilities.maxImageCount);

		VkSwapchainCreateInfoKHR swapchainCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = surface,
			.minImageCount = imageCount,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = swapExtent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
		};

		auto queueFamilyIndices = physicalDevice->GetQueueFamilyIndices(surface);

		if (queueFamilyIndices.GraphicsFamily != queueFamilyIndices.PresentFamily) {
			uint32_t indices[] = { queueFamilyIndices.GraphicsFamily.value(), queueFamilyIndices.PresentFamily.value() };
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = indices;
		}
		else {
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainCreateInfo.queueFamilyIndexCount = 0;
			swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		swapchainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = nullptr;

		if (vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &swapChainHandle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan swap chain.");
		}

		uint32_t swapChainImageCount = 0;
		vkGetSwapchainImagesKHR(logicalDevice, swapChainHandle, &swapChainImageCount, nullptr);
		images.resize(swapChainImageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChainHandle, &swapChainImageCount, &images[0]);

		imageFormat = surfaceFormat.format;
		extent = swapExtent;
	}

	~SwapChain()
	{
		vkDestroySwapchainKHR(logicalDevice, swapChainHandle, nullptr);
	}

	const VkSwapchainKHR& GetHandle() const
	{
		return swapChainHandle;
	}

	const std::vector<VkImage>& GetImages() const
	{
		return images;
	}

	const VkFormat GetImageFormat() const
	{
		return imageFormat;
	}

	const VkExtent2D GetExtent() const
	{
		return extent;
	}
private:
	VkSwapchainKHR swapChainHandle;
	const VkDevice& logicalDevice;
	std::vector<VkImage> images;
	VkFormat imageFormat;
	VkExtent2D extent;
};
