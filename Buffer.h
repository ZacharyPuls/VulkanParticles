#pragma once

#include "GraphicsHeaders.h"
#include "PhysicalDevice.h"
#include "Vertex.h"

#include <stdexcept>
#include <vector>

class Buffer
{
public:

	Buffer(const VkDevice& logicalDevice, PhysicalDevice*& physicalDevice)
		: logicalDevice(logicalDevice),
		  physicalDevice(physicalDevice)
	{
	}

	~Buffer()
	{
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);
		vkDestroyBuffer(logicalDevice, bufferHandle, nullptr);
	}

	void Allocate(const uint32_t usage, const int memoryPropertyFlags, const size_t& size)
	{
		VkBufferCreateInfo bufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};

		if (vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &bufferHandle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan buffer.");
		}

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, bufferHandle, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memoryRequirements.size,
			.memoryTypeIndex = physicalDevice->FindMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags)
		};

		if (vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate memory for Buffer.");
		}

		vkBindBufferMemory(logicalDevice, bufferHandle, bufferMemory, 0);
	}

	void FillData(const size_t& size, const void* data) const
	{
		void* memory;
		vkMapMemory(logicalDevice, bufferMemory, 0, size, 0, &memory);
		memcpy(memory, data, size);
		vkUnmapMemory(logicalDevice, bufferMemory);
	}
	
	const VkBuffer& GetHandle() const
	{
		return bufferHandle;
	}
protected:
	VkBuffer bufferHandle;
	VkDeviceMemory bufferMemory;
	const VkDevice& logicalDevice;
	PhysicalDevice*& physicalDevice;
};
