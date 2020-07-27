#pragma once

#include "stdafx.h"
#include "Util.h"

class Buffer
{
public:
	explicit Buffer(const VulkanDeviceContext& deviceContext,
	                const vk::DeviceSize size,
	                const vk::BufferUsageFlags usageFlags,
	                const vk::SharingMode sharingMode = vk::SharingMode::eExclusive,
	                const vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags()) :
		deviceContext_(deviceContext), usageFlags_(usageFlags),
		memoryFlags_(memoryFlags)
	{
		const vk::BufferCreateInfo bufferCreateInfo({}, size, usageFlags, sharingMode);
		bufferHandle_ = deviceContext_.LogicalDevice->createBuffer(bufferCreateInfo);
		const auto memoryRequirements = deviceContext_.LogicalDevice->getBufferMemoryRequirements(bufferHandle_);

		const vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size,
		                                                Util::FindSupportedMemoryType(deviceContext_,
		                                                                              memoryRequirements.memoryTypeBits,
		                                                                              memoryFlags));

		WRAP_VK_MEMORY_EXCEPTIONS(memoryHandle_ = deviceContext_.LogicalDevice->allocateMemory(memoryAllocateInfo),
		                          "Buffer::Buffer() - vk::Device::allocateMemory()")

		deviceContext_.LogicalDevice->bindBufferMemory(bufferHandle_, memoryHandle_, 0);
	}

	~Buffer()
	{
		if (deviceContext_.LogicalDevice.get() != nullptr) {
			deviceContext_.LogicalDevice->destroyBuffer(bufferHandle_);
			deviceContext_.LogicalDevice->freeMemory(memoryHandle_);
		}
	}

	void Fill(const vk::DeviceSize offset, const vk::DeviceSize size, const void* data) const
	{
		const auto ptr = map_(size, data, offset);
		memcpy(ptr, data, static_cast<size_t>(size));
		unmap_();
	}

	static void Copy(const vk::Buffer& source, const vk::Buffer& destination,
		const vk::DeviceSize size, const VulkanDeviceContext& deviceContext, const std::shared_ptr<vk::CommandPool>& commandPool)
	{
		const auto commandBuffer = Util::BeginOneTimeSubmitCommand(deviceContext, commandPool);

		vk::BufferCopy bufferCopy({}, {}, size);
		commandBuffer.copyBuffer(source, destination, 1, &bufferCopy);

		Util::EndOneTimeSubmitCommand(deviceContext, commandPool, commandBuffer);
	}

	static void Copy(const std::unique_ptr<Buffer>& source, const std::unique_ptr<Buffer>& destination,
	                 const vk::DeviceSize size, const VulkanDeviceContext& deviceContext, const std::shared_ptr<vk::CommandPool>& commandPool)
	{
		const auto commandBuffer = Util::BeginOneTimeSubmitCommand(deviceContext, commandPool);

		vk::BufferCopy bufferCopy({}, {}, size);
		commandBuffer.copyBuffer(source->GetHandle(), destination->GetHandle(), 1, &bufferCopy);

		Util::EndOneTimeSubmitCommand(deviceContext, commandPool, commandBuffer);
	}

	static void Copy(const std::unique_ptr<Buffer>& source, const vk::Image& destination, const vk::Extent3D imageExtent,
	                 const VulkanDeviceContext& deviceContext, const std::shared_ptr<vk::CommandPool>& commandPool)
	{
		const auto commandBuffer = Util::BeginOneTimeSubmitCommand(deviceContext, commandPool);

		vk::BufferImageCopy bufferImageCopy({}, {}, {}, {vk::ImageAspectFlagBits::eColor, {}, {}, 1}, {0, 0, 0},
		                                    imageExtent);
		commandBuffer.copyBufferToImage(source->GetHandle(), destination, vk::ImageLayout::eTransferDstOptimal,
		                                bufferImageCopy);
		Util::EndOneTimeSubmitCommand(deviceContext, commandPool, commandBuffer);
	}

	const vk::Buffer& GetHandle() const
	{
		return bufferHandle_;
	}

	virtual void Bind(const vk::CommandBuffer& commandBuffer) = 0;

protected:
	const VulkanDeviceContext& deviceContext_;

private:
	vk::BufferUsageFlags usageFlags_;
	vk::MemoryPropertyFlags memoryFlags_;
	vk::Buffer bufferHandle_;
	vk::DeviceMemory memoryHandle_;

	void* map_(const vk::DeviceSize size, const void* data, const vk::DeviceSize offset = 0,
	           const vk::MemoryMapFlags flags = vk::MemoryMapFlags()) const
	{
		return deviceContext_.LogicalDevice->mapMemory(memoryHandle_, offset, size, flags);
	}

	void unmap_() const
	{
		deviceContext_.LogicalDevice->unmapMemory(memoryHandle_);
	}
};
