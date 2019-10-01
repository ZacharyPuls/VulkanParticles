#pragma once

#include "GraphicsHeaders.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "GraphicsPipeline.h"

#include <stdexcept>
#include <vector>
#include <string>
#include "VertexBuffer.h"

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

	void SubmitRenderPass(RenderPass*& renderPass, const std::vector<Framebuffer*>& framebuffers, const VkExtent2D& renderAreaExtent, GraphicsPipeline*& graphicsPipeline, VertexBuffer*& vertexBuffer)
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

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = renderPass->GetHandle();
			renderPassBeginInfo.framebuffer = framebuffers[i]->GetHandle();
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = renderAreaExtent;
			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearColor;
			vkCmdBeginRenderPass(commandBufferHandles[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBufferHandles[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetHandle());
			VkBuffer vertexBuffers[] = { vertexBuffer->GetHandle() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBufferHandles[i], 0, 1, vertexBuffers, offsets);
			vkCmdDraw(commandBufferHandles[i], vertexBuffer->GetNumVertices(), 1, 0, 0);
			vkCmdEndRenderPass(commandBufferHandles[i]);

			if (vkEndCommandBuffer(commandBufferHandles[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to record Vulkan command buffer with index [" + std::to_string(i) + "].");
			}
		}
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
