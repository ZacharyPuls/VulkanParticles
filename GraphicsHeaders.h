#pragma once

#include <vulkan/vulkan.hpp>
#include "GLFW/glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

#define STBI_ONLY_JPEG
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

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
	uint32_t FindSupportedMemoryType(const vk::PhysicalDevice* parentPhysicalDevice_, const uint32_t typeFilter,
	                                 const vk::MemoryPropertyFlags properties)
	{
		auto physicalDeviceMemoryProperties = parentPhysicalDevice_->getMemoryProperties();

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

	vk::Format FindSupportedFormat(const vk::PhysicalDevice& physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling imageTiling, vk::FormatFeatureFlags formatFeatureFlags)
	{
		return *std::find_if(candidates.begin(), candidates.end(), [physicalDevice, imageTiling, formatFeatureFlags](auto candidate)
			{
				auto physicalDeviceProperties = physicalDevice.getFormatProperties(candidate);
				return (imageTiling == vk::ImageTiling::eLinear && (physicalDeviceProperties.linearTilingFeatures & formatFeatureFlags) == formatFeatureFlags) ||
					(imageTiling == vk::ImageTiling::eOptimal && (physicalDeviceProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags);
			});
	}

	static vk::CommandBuffer BeginOneTimeSubmitCommand(const vk::CommandPool& commandPool, const vk::Device* logicalDevice)
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo(commandPool, {}, 1);
		vk::CommandBuffer commandBuffer;
		WRAP_VK_MEMORY_EXCEPTIONS(commandBuffer = logicalDevice->allocateCommandBuffers(commandBufferAllocateInfo)[0],
		                          "Util::BeginOneTimeSubmitCommand() - vk::Device::allocateCommandBuffers()")

		vk::CommandBufferBeginInfo commandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(commandBufferBeginInfo);

		return commandBuffer;
	}

	static void EndOneTimeSubmitCommand(const vk::CommandBuffer& commandBuffer, const vk::CommandPool& commandPool, const vk::Device* logicalDevice, const vk::Queue& graphicsQueue)
	{
		commandBuffer.end();

		vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer);
		graphicsQueue.submit(1, &submitInfo, nullptr);
		graphicsQueue.waitIdle();

		logicalDevice->freeCommandBuffers(commandPool, commandBuffer);
	}
};
