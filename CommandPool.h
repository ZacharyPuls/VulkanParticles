#pragma once

#include "GraphicsHeaders.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "GraphicsPipeline.h"
#include "Buffer.h"

#include <stdexcept>
#include <vector>
#include <string>

class CommandPool
{
public:
	CommandPool(const VkDevice& logicalDevice, const uint32_t queueFamilyIndex) : logicalDevice(logicalDevice)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
		commandPoolCreateInfo.flags = 0;

		if (vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, nullptr, &commandPoolHandle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan command pool.");
		}
	}

	~CommandPool()
	{
		FreeBuffers();
		vkDestroyCommandPool(logicalDevice, commandPoolHandle, nullptr);
	}

	const VkCommandPool& GetHandle() const
	{
		return commandPoolHandle;
	}

	void SubmitRenderPass(RenderPass*& renderPass, const std::vector<Framebuffer*>& framebuffers, const VkExtent2D& renderAreaExtent, GraphicsPipeline*& graphicsPipeline, Buffer* vertexBuffer, Buffer* indexBuffer, const uint32_t numIndices)
	{
		commandBufferHandles.resize(framebuffers.size());
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPoolHandle,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<uint32_t>(framebuffers.size())
		};

		if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &commandBufferHandles[0]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate Vulkan command buffers.");
		}
		
		for (size_t i = 0; i < commandBufferHandles.size(); i++) {
			VkCommandBufferBeginInfo commandBufferBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = 0,
				.pInheritanceInfo = nullptr
			};

			if (vkBeginCommandBuffer(commandBufferHandles[i], &commandBufferBeginInfo) != VK_SUCCESS) {
				throw std::runtime_error("Failed to begin recording Vulkan command buffer with index [" + std::to_string(i) + "].");
			}

			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			
			VkRenderPassBeginInfo renderPassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.renderPass = renderPass->GetHandle(),
				.framebuffer = framebuffers[i]->GetHandle(),
				.renderArea =
				{
					.offset = { 0, 0 },
					.extent = renderAreaExtent
				},
				.clearValueCount = 1,
				.pClearValues = &clearColor
			};

			vkCmdBeginRenderPass(commandBufferHandles[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBufferHandles[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetHandle());
			VkBuffer vertexBuffers[] = { vertexBuffer->GetHandle() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBufferHandles[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBufferHandles[i], indexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBufferHandles[i], numIndices, 1, 0, 0, 0);
			vkCmdEndRenderPass(commandBufferHandles[i]);

			if (vkEndCommandBuffer(commandBufferHandles[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to record Vulkan command buffer with index [" + std::to_string(i) + "].");
			}
		}
	}

	void ExecuteBufferCopy(const Buffer* sourceBuffer, const Buffer* destinationBuffer, const VkDeviceSize& size, const VkQueue& graphicsQueue)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPoolHandle,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		VkCommandBuffer commandBufferHandle;
		vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &commandBufferHandle);

		VkCommandBufferBeginInfo commandBufferBeginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};
		
		vkBeginCommandBuffer(commandBufferHandle, &commandBufferBeginInfo);

		VkBufferCopy bufferCopy = {
			.size = size
		};

		vkCmdCopyBuffer(commandBufferHandle, sourceBuffer->GetHandle(), destinationBuffer->GetHandle(), 1, &bufferCopy);
		vkEndCommandBuffer(commandBufferHandle);

		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBufferHandle
		};

		// TODO: if we need to do more transfer operations in parallel, instead of vkQueueWaitIdle, create and wait for a fence for each transfer op
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(logicalDevice, commandPoolHandle, 1, &commandBufferHandle);
	}

	const VkCommandBuffer* GetBuffer(const uint32_t index) const
	{
		return &commandBufferHandles[index];
	}
	
	void FreeBuffers()
	{
		vkFreeCommandBuffers(logicalDevice, commandPoolHandle, static_cast<uint32_t>(commandBufferHandles.size()), &commandBufferHandles[0]);
	}
private:
	VkCommandPool commandPoolHandle;
	std::vector<VkCommandBuffer> commandBufferHandles;
	const VkDevice& logicalDevice;
};
