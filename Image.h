#pragma once

#include "GraphicsHeaders.h"

class Image
{
public:
	Image(std::shared_ptr<vk::Device> logicalDevice, std::shared_ptr<vk::PhysicalDevice> physicalDevice,
	      const std::array<uint32_t, 3> dimensions, const vk::Format& format,
	      const vk::ImageUsageFlags& imageUsageFlags,
	      const vk::MemoryPropertyFlags& memoryPropertyFlags) : parentLogicalDevice_(logicalDevice),
	                                                            parentPhysicalDevice_(physicalDevice), format_(format)
	{
		int width, height, numChannels;
		width_ = static_cast<uint32_t>(dimensions[0]);
		height_ = static_cast<uint32_t>(dimensions[1]);
		mipLevels_ = static_cast<uint32_t>(dimensions[2]);
		imageHandle_ = parentLogicalDevice_->createImage({
			{}, vk::ImageType::e2D, format_, {width_, height_, 1}, mipLevels_, 1,
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
	}

	~Image()
	{
		parentLogicalDevice_->destroyImageView(imageView_);
		parentLogicalDevice_->destroyImage(imageHandle_);
		parentLogicalDevice_->freeMemory(memoryHandle_);
	}

	void CreateImageView(vk::Format format, vk::ImageAspectFlags aspectFlags) 
	{
		imageView_ = parentLogicalDevice_->createImageView({ {}, imageHandle_, vk::ImageViewType::e2D,
			format, {}, {
				aspectFlags, 0, mipLevels_, 0, 1
			} });
	}

	void TransitionLayout(const vk::CommandPool& commandPool, const vk::Queue& graphicsQueue, vk::Format format,
		vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags sourceAccessMask,
		vk::AccessFlags destinationAccessMask, vk::PipelineStageFlags sourceStage,
		vk::PipelineStageFlags destinationStage, vk::ImageAspectFlags aspectFlags)
	{
		const auto commandBuffer = Util::BeginOneTimeSubmitCommand(commandPool, parentLogicalDevice_.get());
		vk::ImageMemoryBarrier imageMemoryBarrier(sourceAccessMask, destinationAccessMask, oldLayout, newLayout, {}, {},
			imageHandle_,
			{ aspectFlags, 0, mipLevels_, 0, 1 });
		commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, imageMemoryBarrier);
		Util::EndOneTimeSubmitCommand(commandBuffer, commandPool, parentLogicalDevice_.get(),
			graphicsQueue);
	}

	vk::ImageView GetImageView() const
	{
		return imageView_;
	}

protected:
	std::shared_ptr<vk::Device> parentLogicalDevice_;
	std::shared_ptr<vk::PhysicalDevice> parentPhysicalDevice_;
	vk::Image imageHandle_;
	vk::DeviceMemory memoryHandle_;
	vk::ImageView imageView_;

	uint32_t width_;
	uint32_t height_;
	uint32_t mipLevels_;
	vk::Format format_;
};
