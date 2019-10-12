#pragma once

#include "GraphicsHeaders.h"
#include "PhysicalDevice.h"
#include "Framebuffer.h"

#include <stdexcept>
#include <vector>
#include <algorithm>
#include "LogicalDevice.h"
#include "Surface.h"

class SwapChain
{
public:
	SwapChain(LogicalDevice* logicalDevice, Surface* surface) : parentLogicalDevice_(logicalDevice), parentSurface_(surface)
	{
		const auto swapChainSupport = SupportDetails::Query(physicalDevice->GetHandle(), surface);
		const auto surfaceFormat = parentSurface_->QuerySurfaceFormat();
		const auto presentMode = parentSurface_->GetPresentModeOrDefault();
		const auto swapExtent = parentSurface_->ClampExtentToSurface()
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

		auto queueFamilyIndices = physicalDevice->EnumerateQueueFamilies(surface);

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

		if (vkCreateSwapchainKHR(logicalDevice->GetHandle(), &swapchainCreateInfo, nullptr, &swapChainHandle_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan swap chain.");
		}

		uint32_t swapChainImageCount = 0;
		vkGetSwapchainImagesKHR(logicalDevice->GetHandle(), swapChainHandle_, &swapChainImageCount, nullptr);
		images_.resize(swapChainImageCount);
		vkGetSwapchainImagesKHR(logicalDevice->GetHandle(), swapChainHandle_, &swapChainImageCount, &images_[0]);

		imageFormat_ = surfaceFormat.format;
		extent_ = swapExtent;
	}

	~SwapChain()
	{
	}

	const vk::SwapchainKHR& GetHandle() const
	{
		return swapChainHandle_;
	}

	const std::vector<vk::Image>& GetImages() const
	{
		return images_;
	}

	const size_t& GetNumImages() const
	{
		return images_.size();
	}

	const vk::Format GetImageFormat() const
	{
		return imageFormat_;
	}

	const VkExtent2D GetExtent() const
	{
		return extent_;
	}
private:
	LogicalDevice* parentLogicalDevice_;
	Surface* parentSurface_;
	vk::SwapchainKHR swapChainHandle_;
	std::vector<vk::Image> images_;
	vk::Format imageFormat_;
	vk::Extent2D extent_;
};
