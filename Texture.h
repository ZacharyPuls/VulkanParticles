#pragma once

#include "stdafx.h"
#include <memory>
#include "Image.h"
#include "VulkanDeviceContext.h"

class Texture : public Image
{
public:
	Texture(const std::string& filename, const VulkanDeviceContext& deviceContext,
	        vk::UniqueCommandPool& commandPool, const bool generateMipmaps) :
		Image(deviceContext, commandPool, getImageDimensions_(filename, generateMipmaps),
		      vk::Format::eR8G8B8A8Unorm,
		      vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::
		      eTransferDst | vk::ImageUsageFlagBits::eSampled,
		      vk::MemoryPropertyFlagBits::eDeviceLocal)
	{
		DebugMessage("Texture::Texture()");
		int width, height, numChannels;
		imagePixels_.reset(stbi_load(filename.c_str(), &width, &height, &numChannels, STBI_rgb_alpha));
		const auto imageSize = static_cast<vk::DeviceSize>(width) * static_cast<vk::DeviceSize>(height) * 4L;
		auto stagingBuffer = std::make_shared<PixelBuffer>(deviceContext_, commandPool_, imageSize,
		                                                   vk::BufferUsageFlagBits::eTransferSrc,
		                                                   vk::SharingMode::eExclusive,
		                                                   vk::MemoryPropertyFlagBits::eHostVisible |
		                                                   vk::MemoryPropertyFlagBits::eHostCoherent, "Texture::stagingBuffer");
		stagingBuffer->Fill(0, imageSize, &imagePixels_.get()[0]);

		TransitionLayout(vk::Format::eR8G8B8A8Unorm,
		                 vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
		                 {}, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTopOfPipe,
		                 vk::PipelineStageFlagBits::eTransfer, vk::ImageAspectFlagBits::eColor);
		stagingBuffer->CopyTo(imageHandle_, vk::Extent3D{
			                      static_cast<uint32_t>(width_), static_cast<uint32_t>(height_), 1
		                      });

		auto commandBuffers = deviceContext_.LogicalDevice->allocateCommandBuffersUnique({ commandPool_.get(), {}, 1 });
		auto& commandBuffer = commandBuffers[0];
		commandBuffer->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		
		vk::ImageMemoryBarrier imageMemoryBarrier({}, {}, {}, {}, {}, {}, imageHandle_.get(),
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

				commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
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

				commandBuffer->blitImage(imageHandle_.get(), vk::ImageLayout::eTransferSrcOptimal, imageHandle_.get(),
				                        vk::ImageLayout::eTransferDstOptimal, 1, &imageBlit, vk::Filter::eLinear);

				imageMemoryBarrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
				imageMemoryBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
				imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

				commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
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

			commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
			                              vk::PipelineStageFlagBits::eFragmentShader,
			                              {}, {}, {}, {imageMemoryBarrier});

			commandBuffer->end();
			vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer.get());
			deviceContext_.GraphicsQueue->submit(1, &submitInfo, nullptr);
			deviceContext_.GraphicsQueue->waitIdle();
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
		samplerHandle_ = deviceContext_.LogicalDevice->createSamplerUnique(samplerCreateInfo);
	}

	~Texture()
	{
		DebugMessage("Texture::~Texture()");
	}

	vk::DescriptorImageInfo GenerateDescriptorImageInfo() const
	{
		return {
			samplerHandle_.get(), imageView_.get(), vk::ImageLayout::eShaderReadOnlyOptimal
		};
	}

private:
	vk::UniqueSampler samplerHandle_;

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
