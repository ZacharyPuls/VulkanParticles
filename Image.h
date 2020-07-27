#pragma once

#include "stdafx.h"
#include "VulkanDeviceContext.h"
#include "Util.h"

class Image
{
public:
	Image(const VulkanDeviceContext& deviceContext, const std::shared_ptr<vk::CommandPool>& commandPool,
	      const std::array<uint32_t, 3> dimensions, const vk::Format& format,
	      const vk::ImageUsageFlags& imageUsageFlags,
	      const vk::MemoryPropertyFlags& memoryPropertyFlags) : deviceContext_(deviceContext),
	                                                            commandPool_(commandPool), format_(format)
	{
		width_ = static_cast<uint32_t>(dimensions[0]);
		height_ = static_cast<uint32_t>(dimensions[1]);
		mipLevels_ = static_cast<uint32_t>(dimensions[2]);
		
		imageHandle_ = deviceContext_.LogicalDevice->createImage({
			{}, vk::ImageType::e2D, format_, {width_, height_, 1}, mipLevels_, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, imageUsageFlags
		});
		const auto memoryRequirements = deviceContext_.LogicalDevice->getImageMemoryRequirements(imageHandle_);
		memoryHandle_ = deviceContext_.LogicalDevice->allocateMemory({
			memoryRequirements.size,
			Util::FindSupportedMemoryType(
				deviceContext_,
				memoryRequirements.memoryTypeBits,
				memoryPropertyFlags)
		});
		deviceContext_.LogicalDevice->bindImageMemory(imageHandle_, memoryHandle_, 0);
	}

	~Image()
	{
		DebugMessage("Cleaning up VulkanParticles::Image.");
		if (deviceContext_.LogicalDevice != nullptr) {
			deviceContext_.LogicalDevice->destroyImageView(imageView_);
			deviceContext_.LogicalDevice->destroyImage(imageHandle_);
			deviceContext_.LogicalDevice->freeMemory(memoryHandle_);
		}
	}

	void CreateImageView(vk::Format format, vk::ImageAspectFlags aspectFlags)
	{
		imageView_ = deviceContext_.LogicalDevice->createImageView({
			{}, imageHandle_, vk::ImageViewType::e2D,
			format, {}, {
				aspectFlags, 0, mipLevels_, 0, 1
			}
		});
	}

	void TransitionLayout(vk::Format format,
	                      vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags sourceAccessMask,
	                      vk::AccessFlags destinationAccessMask, vk::PipelineStageFlags sourceStage,
	                      vk::PipelineStageFlags destinationStage, vk::ImageAspectFlags aspectFlags)
	{
		const auto commandBuffer = Util::BeginOneTimeSubmitCommand(deviceContext_, commandPool_);
		vk::ImageMemoryBarrier imageMemoryBarrier(sourceAccessMask, destinationAccessMask, oldLayout, newLayout, {}, {},
		                                          imageHandle_,
		                                          {aspectFlags, 0, mipLevels_, 0, 1});
		commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, imageMemoryBarrier);
		Util::EndOneTimeSubmitCommand(deviceContext_, commandPool_, commandBuffer);
	}

	vk::ImageView GetImageView() const
	{
		return imageView_;
	}

protected:
	const VulkanDeviceContext& deviceContext_;
	const std::shared_ptr<vk::CommandPool>& commandPool_;
	vk::Image imageHandle_;
	vk::DeviceMemory memoryHandle_;
	vk::ImageView imageView_;

	uint32_t width_;
	uint32_t height_;
	uint32_t mipLevels_;
	vk::Format format_;
};
