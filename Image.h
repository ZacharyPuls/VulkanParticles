#pragma once

#include "stdafx.h"
#include "VulkanDeviceContext.h"
#include "Util.h"

class Image
{
public:
	Image(const VulkanDeviceContext& deviceContext, vk::UniqueCommandPool& commandPool,
	      const std::array<uint32_t, 3> dimensions, const vk::Format& format,
	      const vk::ImageUsageFlags& imageUsageFlags,
	      const vk::MemoryPropertyFlags& memoryPropertyFlags) : deviceContext_(deviceContext),
	                                                            commandPool_(commandPool), format_(format)
	{
		DebugMessage("Image::Image()");
		width_ = static_cast<uint32_t>(dimensions[0]);
		height_ = static_cast<uint32_t>(dimensions[1]);
		mipLevels_ = static_cast<uint32_t>(dimensions[2]);
		
		imageHandle_ = deviceContext_.LogicalDevice->createImageUnique({
			{}, vk::ImageType::e2D, format_, {width_, height_, 1}, mipLevels_, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, imageUsageFlags
		});
		
		const auto memoryRequirements = deviceContext_.LogicalDevice->getImageMemoryRequirements(imageHandle_.get());
		memoryHandle_ = deviceContext_.LogicalDevice->allocateMemoryUnique({
			memoryRequirements.size,
			Util::FindSupportedMemoryType(
				deviceContext_, memoryRequirements.memoryTypeBits, memoryPropertyFlags)
		});
		deviceContext_.LogicalDevice->bindImageMemory(imageHandle_.get(), memoryHandle_.get(), 0);
	}

	~Image()
	{
		DebugMessage("Image::~Image()");
	}

	void CreateImageView(vk::Format format, vk::ImageAspectFlags aspectFlags)
	{
		imageView_ = deviceContext_.LogicalDevice->createImageViewUnique({
			{}, imageHandle_.get(), vk::ImageViewType::e2D, format, {},
			{aspectFlags, 0, mipLevels_, 0, 1}
		});
	}

	void TransitionLayout(vk::Format format,
	                      vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags sourceAccessMask,
	                      vk::AccessFlags destinationAccessMask, vk::PipelineStageFlags sourceStage,
	                      vk::PipelineStageFlags destinationStage, vk::ImageAspectFlags aspectFlags)
	{
		auto commandBuffers = deviceContext_.LogicalDevice->allocateCommandBuffersUnique({ commandPool_.get(), {}, 1 });
		auto& commandBuffer = commandBuffers[0];
		commandBuffer->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		commandBuffer->pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, vk::ImageMemoryBarrier{
			                               sourceAccessMask, destinationAccessMask, oldLayout, newLayout, {}, {},
			                               imageHandle_.get(), {aspectFlags, 0, mipLevels_, 0, 1}
		                               });
		commandBuffer->end();
		vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer.get());
		deviceContext_.GraphicsQueue->submit(1, &submitInfo, nullptr);
		deviceContext_.GraphicsQueue->waitIdle();
	}

	vk::UniqueImageView& GetImageView()
	{
		return imageView_;
	}

protected:
	const VulkanDeviceContext deviceContext_;
	vk::UniqueCommandPool& commandPool_;
	vk::UniqueImage imageHandle_;
	vk::UniqueDeviceMemory memoryHandle_;
	vk::UniqueImageView imageView_;

	uint32_t width_;
	uint32_t height_;
	uint32_t mipLevels_;
	vk::Format format_;
};
