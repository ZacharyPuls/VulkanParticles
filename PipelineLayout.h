#pragma once

#include "GraphicsHeaders.h"

#include <stdexcept>

class PipelineLayout
{
public:
	PipelineLayout(const VkDevice& logicalDevice) : logicalDevice(logicalDevice)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 0,
			.pSetLayouts = nullptr,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr
		};

		if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayoutHandle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan pipeline layout.");
		}
	}

	~PipelineLayout()
	{
		vkDestroyPipelineLayout(logicalDevice, pipelineLayoutHandle, nullptr);
	}

	const VkPipelineLayout& GetHandle() const
	{
		return pipelineLayoutHandle;
	}
private:
	VkPipelineLayout pipelineLayoutHandle;
	const VkDevice& logicalDevice;
};
