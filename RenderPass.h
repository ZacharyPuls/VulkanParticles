#pragma once

#include "GraphicsHeaders.h"

#include <stdexcept>

class RenderPass {
public:
	RenderPass(const VkDevice& logicalDevice, const VkFormat& colorAttachmentDescriptionImageFormat) : logicalDevice(logicalDevice) {
		VkAttachmentDescription colorAttachmentDescription = {
			.format = colorAttachmentDescriptionImageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};

		VkAttachmentReference colorAttachmentReference = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		VkSubpassDescription subpassDescription = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentReference
		};

		VkSubpassDependency subpassDependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		};
		
		VkRenderPassCreateInfo renderPassCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &colorAttachmentDescription,
			.subpassCount = 1,
			.pSubpasses = &subpassDescription,
			.dependencyCount = 1,
			.pDependencies = &subpassDependency
		};

		if (vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, nullptr, &renderPassHandle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan render pass.");
		}
	}

	~RenderPass() {
		vkDestroyRenderPass(logicalDevice, renderPassHandle, nullptr);
	}

	const VkRenderPass& GetHandle() const {
		return renderPassHandle;
	}
private:
	VkRenderPass renderPassHandle;
	const VkDevice& logicalDevice;
};
