#pragma once

#include "stdafx.h"

#include "Buffer.h"

class GenericBuffer : public Buffer
{
public:
	GenericBuffer(const VulkanDeviceContext& deviceContext,
	              const vk::DeviceSize size, vk::BufferUsageFlags usageFlags, vk::SharingMode sharingMode,
	              vk::MemoryPropertyFlags memoryFlags)
		: Buffer(deviceContext, size, usageFlags, sharingMode, memoryFlags)
	{
	}

	void Update(const vk::DeviceSize offset, const vk::DeviceSize size, const void* data) const
	{
		Fill(offset, size, data);
	}

	void Bind(const vk::CommandBuffer& commandBuffer) override
	{
		// NOOP
	};
};
