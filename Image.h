#pragma once

#include "GraphicsHeaders.h"

class Image
{
public:
	Image(std::shared_ptr<vk::Device> logicalDevice, std::shared_ptr<vk::PhysicalDevice> physicalDevice,
	      const uint32_t width, const uint32_t height, const vk::Format& format,
	      const vk::ImageUsageFlags& imageUsageFlags, const vk::ImageAspectFlagBits& imageAspectFlags,
	      const vk::MemoryPropertyFlags& memoryPropertyFlags) : parentLogicalDevice_(logicalDevice),
	                                                            parentPhysicalDevice_(physicalDevice), width_(width),
	                                                            height_(height), format_(format)
	{
		imageHandle_ = parentLogicalDevice_->createImage({
			{}, vk::ImageType::e2D, format_, {width_, height_, 1}, 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, imageUsageFlags
		});
		const auto memoryRequirements = parentLogicalDevice_->getImageMemoryRequirements(imageHandle_);
		memoryHandle_ = parentLogicalDevice_->allocateMemory({
			memoryRequirements.size,
			Util::FindSupportedMemoryType(
				parentPhysicalDevice_.get(),
				memoryRequirements.memoryTypeBits,
				memoryPropertyFlags)
		});
		parentLogicalDevice_->bindImageMemory(imageHandle_, memoryHandle_, 0);
		imageView_ = parentLogicalDevice_->createImageView({
			{}, imageHandle_, vk::ImageViewType::e2D, format, {}, {imageAspectFlags, 0, 1, 0, 1}
		});
	}

	~Image()
	{
		parentLogicalDevice_->destroyImage(imageHandle_);
		parentLogicalDevice_->freeMemory(memoryHandle_);
	}

private:
	std::shared_ptr<vk::Device> parentLogicalDevice_;
	std::shared_ptr<vk::PhysicalDevice> parentPhysicalDevice_;
	vk::Image imageHandle_;
	vk::DeviceMemory memoryHandle_;
	vk::ImageView imageView_;
	uint32_t width_;
	uint32_t height_;
	vk::Format format_;
};
