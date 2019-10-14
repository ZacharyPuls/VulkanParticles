#pragma once

#include "GraphicsHeaders.h"

class Buffer
{
public:
	explicit Buffer(const std::shared_ptr<vk::Device> logicalDeviceHandle,
	                const std::shared_ptr<vk::PhysicalDevice> physicalDeviceHandle,
	                const vk::DeviceSize size,
	                const vk::BufferUsageFlags usageFlags,
	                const vk::SharingMode sharingMode = vk::SharingMode::eExclusive,
	                const vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlags()) :
		parentLogicalDevice_(logicalDeviceHandle), parentPhysicalDevice_(physicalDeviceHandle), usageFlags_(usageFlags),
		memoryFlags_(memoryFlags)
	{
		const vk::BufferCreateInfo bufferCreateInfo({}, size, usageFlags, sharingMode);
		bufferHandle_ = parentLogicalDevice_->createBuffer(bufferCreateInfo);
		const auto memoryRequirements = parentLogicalDevice_->getBufferMemoryRequirements(bufferHandle_);

		const vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size,
		                                                Util::FindSupportedMemoryType(parentPhysicalDevice_.get(),
		                                                                              memoryRequirements.memoryTypeBits,
		                                                                              memoryFlags));

		WRAP_VK_MEMORY_EXCEPTIONS(memoryHandle_ = parentLogicalDevice_->allocateMemory(memoryAllocateInfo),
		                          "Buffer::Buffer() - vk::Device::allocateMemory()")

		parentLogicalDevice_->bindBufferMemory(bufferHandle_, memoryHandle_, 0);
	}

	~Buffer()
	{
		parentLogicalDevice_->destroyBuffer(bufferHandle_);
		parentLogicalDevice_->freeMemory(memoryHandle_);
	}

	void Fill(const vk::DeviceSize offset, const vk::DeviceSize size, const void* data) const
	{
		const auto ptr = map_(size, data, offset);
		memcpy(ptr, data, static_cast<size_t>(size));
		unmap_();
	}

	static void Copy(const std::unique_ptr<Buffer>& source, const std::unique_ptr<Buffer>& destination,
	                 const vk::DeviceSize size, const vk::CommandPool& commandPool, const vk::Queue& graphicsQueue)
	{
		const auto commandBuffer = Util::BeginOneTimeSubmitCommand(commandPool, source->parentLogicalDevice_.get());

		vk::BufferCopy bufferCopy({}, {}, size);
		commandBuffer.copyBuffer(source->GetHandle(), destination->GetHandle(), 1, &bufferCopy);

		Util::EndOneTimeSubmitCommand(commandBuffer, commandPool, source->parentLogicalDevice_.get(), graphicsQueue);
	}

	static void Copy(const std::unique_ptr<Buffer>& source, const vk::Image& destination, const vk::Extent3D imageExtent,
	                 const vk::CommandPool& commandPool, const vk::Queue& graphicsQueue)
	{
		const auto commandBuffer = Util::BeginOneTimeSubmitCommand(commandPool, source->parentLogicalDevice_.get());

		vk::BufferImageCopy bufferImageCopy({}, {}, {}, {vk::ImageAspectFlagBits::eColor, {}, {}, 1}, {0, 0, 0},
		                                    imageExtent);
		commandBuffer.copyBufferToImage(source->GetHandle(), destination, vk::ImageLayout::eTransferDstOptimal,
		                                bufferImageCopy);
		Util::EndOneTimeSubmitCommand(commandBuffer, commandPool, source->parentLogicalDevice_.get(), graphicsQueue);
	}

	const vk::Buffer& GetHandle() const
	{
		return bufferHandle_;
	}

private:
	std::shared_ptr<vk::Device> parentLogicalDevice_;
	std::shared_ptr<vk::PhysicalDevice> parentPhysicalDevice_;
	vk::BufferUsageFlags usageFlags_;
	vk::MemoryPropertyFlags memoryFlags_;
	vk::Buffer bufferHandle_;
	vk::DeviceMemory memoryHandle_;

	void* map_(const vk::DeviceSize size, const void* data, const vk::DeviceSize offset = 0,
	           const vk::MemoryMapFlags flags = vk::MemoryMapFlags()) const
	{
		return parentLogicalDevice_->mapMemory(memoryHandle_, offset, size, flags);
	}

	void unmap_() const
	{
		parentLogicalDevice_->unmapMemory(memoryHandle_);
	}
};
