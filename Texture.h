#pragma once

#include "stdafx.h"
#include <memory>
#include "GenericBuffer.h"
#include "Image.h"
#include "VulkanDeviceContext.h"

class Texture : public Image
{
public:
	Texture(const std::string& filename, const VulkanDeviceContext& deviceContext,
	        const std::shared_ptr<vk::CommandPool>& commandPool, const bool generateMipmaps) :
		Image(deviceContext, commandPool, getImageDimensions_(filename, generateMipmaps),
		      vk::Format::eR8G8B8A8Unorm,
		      vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::
		      eTransferDst | vk::ImageUsageFlagBits::eSampled,
		      vk::MemoryPropertyFlagBits::eDeviceLocal)
	{
		int width, height, numChannels;
		imagePixels_.reset(stbi_load(filename.c_str(), &width, &height, &numChannels, STBI_rgb_alpha));
		const vk::DeviceSize imageSize = height * width * 4;
		std::unique_ptr<Buffer> stagingBuffer;
		stagingBuffer.reset(new GenericBuffer(deviceContext_, imageSize,
		                               vk::BufferUsageFlagBits::eTransferSrc, {},
		                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::
		                               eHostCoherent));
		stagingBuffer->Fill(0, imageSize, &imagePixels_.get()[0]);

		TransitionLayout(vk::Format::eR8G8B8A8Unorm,
		                 vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
		                 {}, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTopOfPipe,
		                 vk::PipelineStageFlagBits::eTransfer, vk::ImageAspectFlagBits::eColor);
		Buffer::Copy(stagingBuffer, imageHandle_, {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_), 1},
		             deviceContext_, commandPool_);
		auto commandBuffer = Util::BeginOneTimeSubmitCommand(deviceContext_, commandPool_);
		vk::ImageMemoryBarrier imageMemoryBarrier({}, {}, {}, {}, {}, {}, imageHandle_,
		                                          {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

		if (generateMipmaps)
		{
			auto mipWidth = width_;
			auto mipHeight = height_;

			for (uint32_t i = 1; i < mipLevels_; i++)
			{
				imageMemoryBarrier.subresourceRange.baseMipLevel = i - 1;
				imageMemoryBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
				imageMemoryBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
				imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
				imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

				commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
				                              vk::PipelineStageFlagBits::eTransfer,
				                              {}, {}, {}, {imageMemoryBarrier});

				vk::ImageBlit imageBlit({vk::ImageAspectFlagBits::eColor, i - 1, 0, 1},
				                        std::array<vk::Offset3D, 2>{
					                        vk::Offset3D(0, 0, 0), vk::Offset3D(mipWidth, mipHeight, 1)
				                        }, {
					                        vk::ImageAspectFlagBits::eColor, i, 0, 1
				                        }, std::array<vk::Offset3D, 2>{
					                        vk::Offset3D(0, 0, 0),
					                        vk::Offset3D(
						                        mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1
					                        )
				                        });

				commandBuffer.blitImage(imageHandle_, vk::ImageLayout::eTransferSrcOptimal, imageHandle_,
				                        vk::ImageLayout::eTransferDstOptimal, 1, &imageBlit, vk::Filter::eLinear);

				imageMemoryBarrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
				imageMemoryBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
				imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

				commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
				                              vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
				                              {imageMemoryBarrier});

				if (mipWidth > 1)
				{
					mipWidth /= 2;
				}

				if (mipHeight > 1)
				{
					mipHeight /= 2;
				}
			}

			imageMemoryBarrier.subresourceRange.baseMipLevel = mipLevels_ - 1;
			imageMemoryBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			imageMemoryBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
			                              vk::PipelineStageFlagBits::eFragmentShader,
			                              {}, {}, {}, {imageMemoryBarrier});

			Util::EndOneTimeSubmitCommand(deviceContext_, commandPool_, commandBuffer);
		}
		CreateImageView(vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);

		TransitionLayout(vk::Format::eR8G8B8A8Unorm,
		                 vk::ImageLayout::eUndefined,
		                 vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eTransferWrite,
		                 vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eTransfer,
		                 vk::PipelineStageFlagBits::eFragmentShader, vk::ImageAspectFlagBits::eColor);

		vk::SamplerCreateInfo samplerCreateInfo({}, vk::Filter::eLinear, vk::Filter::eLinear,
		                                        vk::SamplerMipmapMode::eLinear, {}, {}, {}, {}, VK_TRUE, 16.0f, {}, {},
		                                        {}, static_cast<float>(mipLevels_), vk::BorderColor::eIntOpaqueBlack,
		                                        {});
		samplerHandle_ = deviceContext_.LogicalDevice->createSampler(samplerCreateInfo);
	}

	~Texture()
	{
		DebugMessage("Cleaning up VulkanParticles::Texture.");
		if (deviceContext_.LogicalDevice != nullptr) {
			deviceContext_.LogicalDevice->destroySampler(samplerHandle_);
		}
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
	vk::Sampler samplerHandle_;

	struct freeImage_
	{
		void operator()(unsigned char* ptr) const
		{
			stbi_image_free(ptr);
		}
	};

	std::unique_ptr<unsigned char, freeImage_> imagePixels_;

	const std::array<uint32_t, 3> getImageDimensions_(const std::string& filename, bool generateMipmaps)
	{
		int width, height, numChannels;
		stbi_info(filename.c_str(), &width, &height, &numChannels);
		return {
			static_cast<uint32_t>(width), static_cast<uint32_t>(height),
			generateMipmaps ? 
			        static_cast<uint32_t>(std::floor(std::log2(std::max(static_cast<uint32_t>(width), static_cast<uint32_t>(height))))) + 1 : 1
			/*                                                                                             /\                                   /\
			   FUN FACT: I HAD UNDERSCORES AFTER THESE PARAMETERS, AND IT MADE ME STAY UP AN EXTRA TWO HOURS AFTER 0100 TO FIGURE OUT WHY MY TEXTURE WOULDN'T CREATE */
		};
	}
};
