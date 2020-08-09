#pragma once

#include "stdafx.h"
#include "Util.h"

class Buffer
{
public:
	explicit Buffer(const VulkanDeviceContext deviceContext,
	                vk::UniqueCommandPool& commandPool,
	                const vk::DeviceSize size,
	                const vk::BufferUsageFlags usageFlags,
	                const vk::SharingMode sharingMode = vk::SharingMode::eExclusive,
	                const vk::MemoryPropertyFlags memoryFlags = {}, const std::string& debugName = "") :
		deviceContext_(deviceContext), commandPool_(commandPool), bufferSize_(size), usageFlags_(usageFlags),
		memoryFlags_(memoryFlags), debugName_(debugName)
	{
		DebugMessage("Buffer::Buffer(\"" + debugName + "\")");
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
		DebugMessage("Buffer::~Buffer(\"" + debugName_ + "\")");
		
		if (deviceContext_.LogicalDevice != nullptr) {
			deviceContext_.LogicalDevice->destroyBuffer(bufferHandle_);
			deviceContext_.LogicalDevice->freeMemory(memoryHandle_);
		}
	}

	void Fill(const vk::DeviceSize offset, const vk::DeviceSize size, const void* data) const
	{
		DebugMessage("Buffer::Fill(\"" + debugName_ + "\")");
		// TODO: [zpuls 2020-07-31T17:23] Figure out a better way to do dynamic buffer resizing/mapping and element insertion/removal, \
											instead of just creating a static buffer with a set size, and hoping the user always passes in the correct amount of data.
		const auto ptr = map_(size, data, offset);
		memcpy(ptr, data, static_cast<size_t>(size));
		unmap_();
	}

	void CopyTo(const std::shared_ptr<Buffer>& destination) const
	{
		DebugMessage("Buffer::CopyTo(\"" + debugName_ + "\", \"" + destination->debugName_ + "\" Buffer<T>)");
		// TODO: [zpuls 2020-07-31T17:40] Add error-handling for buffer destination that does not have the same size.
		auto commandBuffers = deviceContext_.LogicalDevice->allocateCommandBuffersUnique({ commandPool_.get(), {}, 1 });
		auto& commandBuffer = commandBuffers[0];
		commandBuffer->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

		vk::BufferCopy bufferCopy({}, {}, bufferSize_);
		commandBuffer->copyBuffer(bufferHandle_, destination->GetHandle(), 1, &bufferCopy);

		commandBuffer->end();
		vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer.get());
		deviceContext_.GraphicsQueue->submit(1, &submitInfo, nullptr);
		deviceContext_.GraphicsQueue->waitIdle();
	}

	void CopyTo(vk::UniqueImage& destination, const vk::Extent3D imageExtent) const
	{
		DebugMessage("Buffer::CopyTo(\"" + debugName_ + "\", vk::Image)");
		// TODO: [zpuls 2020-07-31T17:41] Add error-handling for image destinations that don't have the correct size (whatever that means in the context of vk::BufferImageCopy).
		auto commandBuffers = deviceContext_.LogicalDevice->allocateCommandBuffersUnique({ commandPool_.get(), {}, 1 });
		auto& commandBuffer = commandBuffers[0];
		commandBuffer->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

		vk::BufferImageCopy bufferImageCopy({}, {}, {}, {vk::ImageAspectFlagBits::eColor, {}, {}, 1}, {0, 0, 0},
		                                    imageExtent);
		commandBuffer->copyBufferToImage(bufferHandle_, destination.get(), vk::ImageLayout::eTransferDstOptimal,
		                                 bufferImageCopy);
		
		commandBuffer->end();
		vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer.get());
		deviceContext_.GraphicsQueue->submit(1, &submitInfo, nullptr);
		deviceContext_.GraphicsQueue->waitIdle();
	}

	const vk::Buffer& GetHandle() const
	{
		return bufferHandle_;
	}

	const vk::DescriptorBufferInfo GenerateDescriptorBufferInfo() const
	{
		return { bufferHandle_, 0, bufferSize_ };
	}

	const size_t GetNumElements() const
	{
		// TODO: [zpuls 2020-07-31T17:22] See Buffer::Fill(), figure out a better way to do dynamic buffers. Buffer::bufferSize_ is not super accurate.
		return 0;
	}

	// virtual void Bind(const vk::CommandBuffer& commandBuffer) = 0;
	void Bind(vk::UniqueCommandBuffer& commandBuffer)
	{
		DebugMessage("Default Buffer::Bind(\"" + debugName_ + "\")");
		// Default NOOP
	}
	
protected:
	const VulkanDeviceContext deviceContext_;
	const vk::UniqueCommandPool& commandPool_;
	vk::DeviceSize bufferSize_;
	vk::BufferUsageFlags usageFlags_;
	vk::MemoryPropertyFlags memoryFlags_;
	std::string debugName_ = "";
	vk::Buffer bufferHandle_;
	vk::DeviceMemory memoryHandle_;

	void* map_(const vk::DeviceSize size, const void* data, const vk::DeviceSize offset = 0,
	           const vk::MemoryMapFlags flags = vk::MemoryMapFlags()) const
	{
		DebugMessage("Buffer::map_(\"" + debugName_ + "\")");
		return deviceContext_.LogicalDevice->mapMemory(memoryHandle_, offset, size, flags);
	}

	void unmap_() const
	{
		DebugMessage("Buffer::unmap_(\"" + debugName_ + "\")");
		deviceContext_.LogicalDevice->unmapMemory(memoryHandle_);
	}
};

class VertexBuffer : public Buffer
{
public:

	VertexBuffer(const VulkanDeviceContext& deviceContext, vk::UniqueCommandPool& commandPool,
		const vk::DeviceSize size, const vk::BufferUsageFlags& usageFlags, const vk::SharingMode sharingMode,
		const vk::MemoryPropertyFlags& memoryFlags, const std::string& debugName)
		: Buffer(deviceContext, commandPool, size, usageFlags, sharingMode, memoryFlags, debugName)
	{
	}
	
	void Bind(vk::UniqueCommandBuffer& commandBuffer)
	{
		DebugMessage("VertexBuffer::Bind(\"" + debugName_ + "\")");
		commandBuffer->bindVertexBuffers(0, bufferHandle_, static_cast<vk::DeviceSize>(0));
	}

	const size_t GetNumElements() const
	{
		return bufferSize_ / sizeof(Vertex);
	}
};

class IndexBuffer : public Buffer
{
public:
	IndexBuffer(const VulkanDeviceContext& deviceContext, vk::UniqueCommandPool& commandPool,
		const vk::DeviceSize size, const vk::BufferUsageFlags& usageFlags, const vk::SharingMode sharingMode,
		const vk::MemoryPropertyFlags& memoryFlags, const std::string& debugName)
		: Buffer(deviceContext, commandPool, size, usageFlags, sharingMode, memoryFlags, debugName)
	{
	}

	void Bind(vk::UniqueCommandBuffer& commandBuffer)
	{
		DebugMessage("IndexBuffer::Bind(\"" + debugName_ + "\")");
		commandBuffer->bindIndexBuffer(bufferHandle_, static_cast<vk::DeviceSize>(0), vk::IndexType::eUint32);
	}

	size_t GetNumElements() const
	{
		return bufferSize_ / sizeof(uint32_t);
	}
};

class PixelBuffer : public Buffer
{
public:

	PixelBuffer(const VulkanDeviceContext& deviceContext, vk::UniqueCommandPool& commandPool,
		const vk::DeviceSize size, const vk::BufferUsageFlags& usageFlags, const vk::SharingMode sharingMode,
		const vk::MemoryPropertyFlags& memoryFlags, const std::string& debugName)
		: Buffer(deviceContext, commandPool, size, usageFlags, sharingMode, memoryFlags, debugName)
	{
	}

	void Bind(vk::UniqueCommandBuffer& commandBuffer) {
		// NOOP
	}
};

class GenericBuffer : public Buffer
{
public:

	GenericBuffer(const VulkanDeviceContext& deviceContext, vk::UniqueCommandPool& commandPool,
		const vk::DeviceSize size, const vk::BufferUsageFlags& usageFlags, const vk::SharingMode sharingMode,
		const vk::MemoryPropertyFlags& memoryFlags, const std::string& debugName)
		: Buffer(deviceContext, commandPool, size, usageFlags, sharingMode, memoryFlags, debugName)
	{
	}

	void Bind(vk::UniqueCommandBuffer& commandBuffer) {
		// NOOP
	}
};