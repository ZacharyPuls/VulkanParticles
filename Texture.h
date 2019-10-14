#pragma once

#include "GraphicsHeaders.h"
#include <memory>
#include "Buffer.h"

class Texture
{
public:
	Texture(const std::string& filename, std::shared_ptr<vk::Device> logicalDevice,
	        std::shared_ptr<vk::PhysicalDevice> physicalDevice, const vk::CommandPool& commandPool,
	        const vk::Queue& graphicsQueue) : parentLogicalDevice_(logicalDevice),
	                                          parentPhysicalDevice_(physicalDevice), parentCommandPool_(commandPool),
	                                          parentGraphicsQueue_(graphicsQueue)
	{
		imagePixels_.reset(stbi_load(filename.c_str(), &width_, &height_, &numChannels_, 4));
		const vk::DeviceSize imageSize = width_ * height_ * numChannels_;
		std::unique_ptr<Buffer> stagingBuffer;
		stagingBuffer.reset(new Buffer(parentLogicalDevice_, parentPhysicalDevice_, imageSize,
		                               vk::BufferUsageFlagBits::eTransferSrc, {},
		                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::
		                               eHostCoherent));
		stagingBuffer->Fill(0, imageSize, &imagePixels_.get()[0]);
		const vk::ImageCreateInfo imageCreateInfo({}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
		                                          {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_), 1}, 1,
		                                          1,
		                                          vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
		                                          vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::
		                                          eSampled,
		                                          {});
		WRAP_VK_MEMORY_EXCEPTIONS(imageHandle_ = parentLogicalDevice_->createImage(imageCreateInfo),
		                          "Texture::Texture() - vk::Device::createImage")

		const auto memoryRequirements = parentLogicalDevice_->getImageMemoryRequirements(imageHandle_);
		const vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size,
		                                                Util::FindSupportedMemoryType(
			                                                parentPhysicalDevice_.get(),
			                                                memoryRequirements.memoryTypeBits,
			                                                vk::MemoryPropertyFlagBits::eDeviceLocal));

		WRAP_VK_MEMORY_EXCEPTIONS(memoryHandle_ = parentLogicalDevice_->allocateMemory(memoryAllocateInfo),
		                          "Texture::Texture() - vk::Device::allocateMemory()")

		parentLogicalDevice_->bindImageMemory(imageHandle_, memoryHandle_, 0);

		TransitionLayout(vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		Buffer::Copy(stagingBuffer, imageHandle_, {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_), 1}, parentCommandPool_, parentGraphicsQueue_);
		TransitionLayout(vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal,
		                 vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	~Texture()
	{
		parentLogicalDevice_->destroyImage(imageHandle_);
	}

	void TransitionLayout(vk::Format format,
	                      vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
	{
		const auto commandBuffer = Util::BeginOneTimeSubmitCommand(parentCommandPool_, parentLogicalDevice_.get());
		vk::ImageMemoryBarrier imageMemoryBarrier({}, {}, oldLayout, newLayout, {}, {}, imageHandle_,
		                                          {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
		commandBuffer.pipelineBarrier({}, {}, {}, {}, {}, imageMemoryBarrier);
		Util::EndOneTimeSubmitCommand(commandBuffer, parentCommandPool_, parentLogicalDevice_.get(),
		                              parentGraphicsQueue_);
	}

private:
	std::shared_ptr<vk::Device> parentLogicalDevice_;
	std::shared_ptr<vk::PhysicalDevice> parentPhysicalDevice_;
	vk::CommandPool parentCommandPool_;
	vk::Queue parentGraphicsQueue_;

	struct freeImage_
	{
		void operator()(unsigned char* ptr) const
		{
			stbi_image_free(ptr);
		}
	};

	std::unique_ptr<unsigned char, freeImage_> imagePixels_;
	vk::Image imageHandle_;
	vk::DeviceMemory memoryHandle_;
	int width_;
	int height_;
	int numChannels_;
};
