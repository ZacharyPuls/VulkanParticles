#pragma once

#include "GraphicsHeaders.h"

class UniformDescriptor
{
public:
	UniformDescriptor(const uint32_t binding, const uint32_t descriptorCount, const vk::DescriptorType type,
	                  const vk::ShaderStageFlags shaderStage) : binding_(binding), descriptorCount_(descriptorCount),
	                                                            type_(type), shaderStage_(shaderStage)
	{
	}

	~UniformDescriptor() = default;

	vk::WriteDescriptorSet GenerateWriteDescriptorSet(const vk::DescriptorSet& descriptorSet,
	                                                  vk::DescriptorImageInfo imageInfo)
	{
		return generateWriteDescriptorSet_(descriptorSet, imageInfo);
	}

	vk::WriteDescriptorSet GenerateWriteDescriptorSet(const vk::DescriptorSet& descriptorSet,
	                                                  vk::DescriptorBufferInfo bufferInfo)
	{
		return generateWriteDescriptorSet_(descriptorSet, {}, bufferInfo);
	}

private:
	uint32_t binding_;
	uint32_t descriptorCount_;
	vk::DescriptorType type_;
	vk::ShaderStageFlags shaderStage_;

	vk::WriteDescriptorSet generateWriteDescriptorSet_(const vk::DescriptorSet& descriptorSet,
	                                                   const vk::DescriptorImageInfo imageInfo = {},
	                                                   const vk::DescriptorBufferInfo bufferInfo = {})
	{
		return {
			descriptorSet, 0, 0, 1, type_, &imageInfo, &bufferInfo
		};
	}
};
