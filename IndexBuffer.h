#pragma once

#include "stdafx.h"

#include <vector>
#include "Buffer.h"

// CPU-visible Index Buffer
class IndexBuffer : public Buffer
{
public:
	IndexBuffer(const VulkanDeviceContext& deviceContext, const std::shared_ptr<vk::CommandPool>& commandPool, const std::vector<uint32_t> indices)
		: Buffer(deviceContext, sizeof(indices[0]) * indices.size(),
		         vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, {},
		         vk::MemoryPropertyFlagBits::eDeviceLocal), commandPool_(commandPool), numIndices_(indices.size())
	{
		Update(indices);
	}

	size_t GetNumIndices() const
	{
		return numIndices_;
	}

	void Update(const vk::DeviceSize offset, const std::vector<uint32_t>& indices) const
	{
		auto bufferSize = sizeof(indices[0]) * indices.size();

		auto stagingBuffer = std::make_unique<GenericBuffer>(deviceContext_, bufferSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::SharingMode::eExclusive,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::
			MemoryPropertyFlagBits::eHostCoherent);
		stagingBuffer->Update(0, bufferSize, &indices[0]);
		Copy(stagingBuffer->GetHandle(), GetHandle(), bufferSize, deviceContext_, commandPool_);
	}

	void Update(const std::vector<uint32_t>& indices) const
	{
		Update(0, indices);
	}

	void Bind(const vk::CommandBuffer& commandBuffer) override
	{
		commandBuffer.bindIndexBuffer(GetHandle(), static_cast<vk::DeviceSize>(0), vk::IndexType::eUint32);
	};

private:
	const std::shared_ptr<vk::CommandPool>& commandPool_;
	const size_t numIndices_;
};
