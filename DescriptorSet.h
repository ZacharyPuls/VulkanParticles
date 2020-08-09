#pragma once

#include "stdafx.h"
#include "VulkanDeviceContext.h"

class DescriptorSet
{
public:
	DescriptorSet(const VulkanDeviceContext deviceContext, vk::UniqueDescriptorPool& descriptorPool,
	              const uint32_t size, vk::DescriptorSetLayout descriptorSetLayoutTemplate) : deviceContext_(
		deviceContext)
	{
		DebugMessage("DescriptorSet::DescriptorSet()");
		std::vector<vk::DescriptorSetLayout> descriptorSetLayout(size, descriptorSetLayoutTemplate);
		DescriptorSets = deviceContext_.LogicalDevice->allocateDescriptorSetsUnique({
			*descriptorPool, size, &descriptorSetLayout[0]
		});
	}

	~DescriptorSet()
	{
		DebugMessage("DescriptorSet::~DescriptorSet()");
	}
	std::vector<vk::UniqueDescriptorSet> DescriptorSets;

	void Update(std::vector<vk::WriteDescriptorSet>& descriptorWrites)
	{
		DebugMessage("DescriptorSet::Update()");
		for (const auto& descriptorSet : DescriptorSets)
		{
			for (auto& descriptorWrite : descriptorWrites)
			{
				descriptorWrite.dstSet = descriptorSet.get();
			}
			deviceContext_.LogicalDevice->updateDescriptorSets(descriptorWrites, nullptr);
		}
	}

	void Bind(const uint32_t frameIndex, vk::UniquePipelineLayout& pipelineLayout, vk::UniqueCommandBuffer& commandBuffer)
	{
		DebugMessage("DescriptorSet::Bind()");
		commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout.get(), 0, 1, &(*DescriptorSets[frameIndex]), 0, nullptr);
	}

private:
	const VulkanDeviceContext deviceContext_;
};
