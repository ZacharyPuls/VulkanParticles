#pragma once

#include "GraphicsHeaders.h"
#include "Vertex.h"
#include "PhysicalDevice.h"
#include "Buffer.h"

#include <vector>

// This class is a noop for now, don't think I really need anything more specific than a ::Buffer
class VertexBuffer : public Buffer {
public:
	VertexBuffer(const VkDevice& logicalDevice, PhysicalDevice*& physicalDevice,
	             const std::vector<Vertex>& vertices) : Buffer(logicalDevice, physicalDevice), vertices(vertices)
	{
		
	}

	~VertexBuffer() = default;

	const uint32_t GetNumVertices() const
	{
		return vertices.size();
	}
private:
	const std::vector<Vertex>& vertices;
};
