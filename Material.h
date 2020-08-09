#pragma once

#include "stdafx.h"
#include "Buffer.h"
#include "Texture.h"

class Material
{
public:
	Material(const VulkanDeviceContext deviceContext, vk::UniqueCommandPool& commandPool,
	         const uint32_t maxFramesInFlight, const std::shared_ptr<Texture>& texture) : deviceContext_(deviceContext),
	                                                                                      commandPool_(commandPool),
	                                                                                      texture_(texture)
	{
		
 

		// TODO: [zpuls 2020-08-02T22:33] Allow dynamic UBO binding, don't hard-code the size of the UBO.
		const auto bufferSize = sizeof(glm::mat4) * 3UL;

		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			uniformBuffers_.emplace_back(new GenericBuffer(deviceContext_, commandPool_, bufferSize,
			                                               vk::BufferUsageFlagBits::eUniformBuffer,
			                                               vk::SharingMode::eExclusive,
			                                               vk::MemoryPropertyFlagBits::eHostVisible |
			                                               vk::MemoryPropertyFlagBits::eHostCoherent,
			                                               "Material::uniformBuffers_[" + std::to_string(i) + "]"));
		}
	}

private:
	const VulkanDeviceContext& deviceContext_;
	vk::UniqueCommandPool& commandPool_;
	std::vector<std::shared_ptr<GenericBuffer>> uniformBuffers_;
	// TODO: [zpuls 2020-08-02T22:21] Support for more than one ::Texture, and the ability to dynamically bind vertex/fragment input 
	const std::shared_ptr<Texture>& texture_;
};
