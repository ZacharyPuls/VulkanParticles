#pragma once

#include "GraphicsHeaders.h"
#include "Vertex.h"
#include "PhysicalDevice.h"

#include <stdexcept>
#include <vector>

class VertexBuffer {
public:
	VertexBuffer(const VkDevice& logicalDevice, const PhysicalDevice*& physicalDevice, const std::vector<Vertex>& vertices) : logicalDevice(logicalDevice), physicalDevice(physicalDevice), vertices(vertices) {
		VkBufferCreateInfo bufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(vertices[0]) * vertices.size(),
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};

		if (vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &bufferHandle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan vertex buffer.");
		}
		
		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, bufferHandle, &memoryRequirements);

		auto memoryProperties = physicalDevice->GetMemoryProperties();
		uint32_t suitableMemoryType = UINT32_MAX;
		uint32_t memoryTypeFilter = memoryRequirements.memoryTypeBits;
		VkMemoryPropertyFlags propertyFilter = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
		{
			if ((memoryTypeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFilter) == propertyFilter)
			{
				suitableMemoryType = i;
			}
		}

		if (suitableMemoryType == UINT32_MAX)
		{
			throw std::runtime_error("Failed to find suitable memory type for VertexBuffer.");
		}

		VkMemoryAllocateInfo memoryAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memoryRequirements.size,
			.memoryTypeIndex = suitableMemoryType
		};

		if (vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate memory for VertexBuffer.");
		}

		vkBindBufferMemory(logicalDevice, bufferHandle, bufferMemory, 0);

		void* data;
		vkMapMemory(logicalDevice, bufferMemory, 0, bufferCreateInfo.size, 0, &data);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferCreateInfo.size));
	}

	~VertexBuffer() {
		vkDestroyBuffer(logicalDevice, bufferHandle, nullptr);
		vkFreeMemory(logicalDevice, bufferMemory, nullptr);
	}
private:
	VkBuffer bufferHandle;
	VkDeviceMemory bufferMemory;
	const VkDevice& logicalDevice;
	const PhysicalDevice*& physicalDevice;
	const std::vector<Vertex>& vertices;
};
