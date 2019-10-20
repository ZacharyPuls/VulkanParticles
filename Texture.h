#pragma once

#include "GraphicsHeaders.h"
#include <memory>
#include "Buffer.h"
#include "Image.h"

class Texture : public Image
{
public:
	Texture(const std::string& filename, std::shared_ptr<vk::Device> logicalDevice,
	        std::shared_ptr<vk::PhysicalDevice> physicalDevice, const vk::CommandPool& commandPool,
	        const vk::Queue& graphicsQueue) : parentCommandPool_(commandPool),
	                                          parentGraphicsQueue_(graphicsQueue), Image(logicalDevice, physicalDevice, getImageDimensions_(filename), vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal)
	{
		int width, height, numChannels;
		imagePixels_.reset(stbi_load(filename.c_str(), &width, &height, &numChannels, 4));
		width_ = width;
		height_ = height;
		const vk::DeviceSize imageSize = width * height * 4;
		std::unique_ptr<Buffer> stagingBuffer;
		stagingBuffer.reset(new Buffer(parentLogicalDevice_, parentPhysicalDevice_, imageSize,
		                               vk::BufferUsageFlagBits::eTransferSrc, {},
		                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::
		                               eHostCoherent));
		stagingBuffer->Fill(0, imageSize, &imagePixels_.get()[0]);

		TransitionLayout(parentCommandPool_, parentGraphicsQueue_, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
		                 {}, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTopOfPipe,
		                 vk::PipelineStageFlagBits::eTransfer);
		Buffer::Copy(stagingBuffer, imageHandle_, {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_), 1},
		             parentCommandPool_, parentGraphicsQueue_);
		TransitionLayout(parentCommandPool_, parentGraphicsQueue_, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal,
		                 vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eTransferWrite,
		                 vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eTransfer,
		                 vk::PipelineStageFlagBits::eFragmentShader);

		CreateImageView(vk::ImageAspectFlagBits::eColor);

		vk::SamplerCreateInfo samplerCreateInfo({}, vk::Filter::eLinear, vk::Filter::eLinear,
		                                        vk::SamplerMipmapMode::eLinear, {}, {}, {}, {}, VK_TRUE, 16.0f, {}, {},
		                                        {}, {}, vk::BorderColor::eIntOpaqueBlack, {});
		samplerHandle_ = parentLogicalDevice_->createSampler(samplerCreateInfo);
	}

	~Texture()
	{
		parentLogicalDevice_->destroySampler(samplerHandle_);
	}

	vk::DescriptorImageInfo GenerateDescriptorImageInfo() const
	{
		return {
			samplerHandle_, imageView_, vk::ImageLayout::eShaderReadOnlyOptimal
		};
	}
	
	const vk::Sampler& GetHandle() const
	{
		return samplerHandle_;
	}

	const vk::ImageView& GetImageView() const
	{
		return imageView_;
	}
	
private:
	vk::CommandPool parentCommandPool_;
	vk::Queue parentGraphicsQueue_;

	vk::Sampler samplerHandle_;
	struct freeImage_
	{
		void operator()(unsigned char* ptr) const
		{
			stbi_image_free(ptr);
		}
	};

	std::unique_ptr<unsigned char, freeImage_> imagePixels_;

	const std::array<uint32_t, 2> getImageDimensions_(const std::string& filename) {
		int width, height, numChannels;
		stbi_info(filename.c_str(), &width, &height, &numChannels);
		return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	}
};
