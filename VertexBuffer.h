#pragma once

#include "stdafx.h"
#include "Vertex.h"

#include <vector>

#include "Buffer.h"

// CPU-visible Vertex Buffer
class VertexBuffer : public Buffer
{
public:
	VertexBuffer(const VulkanDeviceContext& deviceContext, const std::shared_ptr<vk::CommandPool>& commandPool,
	             const std::vector<Vertex>& vertices)
		: Buffer(deviceContext, sizeof(vertices[0]) * vertices.size(),
		         vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		         {}, vk::MemoryPropertyFlagBits::eDeviceLocal), commandPool_(commandPool)
	{
		Update(vertices);
	}

	void Update(const vk::DeviceSize offset, const std::vector<Vertex>& vertices) const
	{
		auto bufferSize = sizeof(vertices[0]) * vertices.size();

		auto stagingBuffer = std::make_unique<GenericBuffer>(deviceContext_, bufferSize,
		                                                     vk::BufferUsageFlagBits::eTransferSrc,
		                                                     vk::SharingMode::eExclusive,
		                                                     vk::MemoryPropertyFlagBits::eHostVisible | vk::
		                                                     MemoryPropertyFlagBits::eHostCoherent);
		stagingBuffer->Update(0, bufferSize, &vertices[0]);
		Copy(stagingBuffer->GetHandle(), GetHandle(), bufferSize, deviceContext_, commandPool_);
	}

	void Update(const std::vector<Vertex>& vertices) const
	{
		Update(0, vertices);
	}

	void Bind(const vk::CommandBuffer& commandBuffer) override
	{
		commandBuffer.bindVertexBuffers(0, GetHandle(), static_cast<vk::DeviceSize>(0));
	};

private:
	const std::shared_ptr<vk::CommandPool>& commandPool_;
};
