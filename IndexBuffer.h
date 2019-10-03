#pragma once

#include "GraphicsHeaders.h"
#include "Buffer.h"

#include <vector>

// This class is a noop for now, don't think I really need anything more specific than a ::Buffer
class IndexBuffer : public Buffer
{
public:
	IndexBuffer(const VkDevice& logicalDevice, PhysicalDevice*& physicalDevice, const std::vector<uint32_t>& indices)
		: Buffer(logicalDevice, physicalDevice),
		  indices(indices)
	{
		// Allocate(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices.size() * sizeof(indices[0]), &indices[0]);
	}
	

private:
	const std::vector<uint32_t>& indices;
};
