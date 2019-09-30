#pragma once

#include "GraphicsHeaders.h"

#include <stdexcept>

class ImageView
{
public:
	ImageView(const VkDevice& logicalDevice, const VkImage& image, const VkFormat& imageFormat) : logicalDevice(logicalDevice), image(image)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = imageFormat,
			.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		if (vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageViewHandle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan image view.");
		}
	}

	~ImageView()
	{
		vkDestroyImageView(logicalDevice, imageViewHandle, nullptr);
	}

	const VkImageView& GetHandle() const
	{
		return imageViewHandle;
	}
private:
	VkImageView imageViewHandle;
	const VkDevice& logicalDevice;
	const VkImage& image;
};
