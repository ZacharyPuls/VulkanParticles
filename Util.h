#pragma once

#include "VulkanDeviceContext.h"

#define WRAP_VK_MEMORY_EXCEPTIONS(x, msg)																			\
		try																											\
		{																											\
			x;																										\
		}																											\
		catch (vk::OutOfHostMemoryError & e)																		\
		{																											\
			throw std::runtime_error("Error when executing " + std::string(msg) +									\
									", a host memory allocation has failed.");										\
		}																											\
		catch (vk::OutOfDeviceMemoryError & e)																		\
		{																											\
			throw std::runtime_error("Error when executing " + std::string(msg) +									\
									", a device memory allocation has failed.");									\
		}

namespace Util
{
	static uint32_t FindSupportedMemoryType(const VulkanDeviceContext& deviceContext, const uint32_t typeFilter,
		const vk::MemoryPropertyFlags properties)
	{
		auto physicalDeviceMemoryProperties = deviceContext.PhysicalDevice->getMemoryProperties();

		for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) ==
				properties)
			{
				return i;
			}
		}

		throw std::runtime_error(
			"Could not find supported vk::MemoryType that matches given filter: type=" + std::to_string(typeFilter) +
			", properties=" + std::to_string(static_cast<uint32_t>(properties)) + ".");
	}

	static vk::Format FindSupportedFormat(const VulkanDeviceContext& deviceContext, const std::vector<vk::Format>& candidates, vk::ImageTiling imageTiling, vk::FormatFeatureFlags formatFeatureFlags)
	{
		const auto physicalDevice = *deviceContext.PhysicalDevice;
		return *std::find_if(candidates.begin(), candidates.end(), [physicalDevice, imageTiling, formatFeatureFlags](auto candidate)
			{
				auto physicalDeviceProperties = physicalDevice.getFormatProperties(candidate);
				return (imageTiling == vk::ImageTiling::eLinear && (physicalDeviceProperties.linearTilingFeatures & formatFeatureFlags) == formatFeatureFlags) ||
					(imageTiling == vk::ImageTiling::eOptimal && (physicalDeviceProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags);
			});
	}

	static vk::CommandBuffer BeginOneTimeSubmitCommand(const VulkanDeviceContext& deviceContext, const std::shared_ptr<vk::CommandPool>& commandPool)
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*commandPool, {}, 1);
		vk::CommandBuffer commandBuffer;
		WRAP_VK_MEMORY_EXCEPTIONS(commandBuffer = deviceContext.LogicalDevice->allocateCommandBuffers(commandBufferAllocateInfo)[0],
			"Util::BeginOneTimeSubmitCommand() - vk::Device::allocateCommandBuffers()")

			vk::CommandBufferBeginInfo commandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(commandBufferBeginInfo);

		return commandBuffer;
	}

	static void EndOneTimeSubmitCommand(const VulkanDeviceContext& deviceContext, const std::shared_ptr<vk::CommandPool>& commandPool, const vk::CommandBuffer& commandBuffer)
	{
		commandBuffer.end();

		vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer);
		auto result = deviceContext.GraphicsQueue->submit(1, &submitInfo, nullptr);
		if (result != vk::Result::eSuccess)
		{
			std::cerr << "[Util::EndOneTimeSubmitCommand] Could not submit one-time command to graphics queue. Result was " << result << std::endl;
		}
		deviceContext.GraphicsQueue->waitIdle();

		deviceContext.LogicalDevice->freeCommandBuffers(*commandPool, commandBuffer);
	}
};
