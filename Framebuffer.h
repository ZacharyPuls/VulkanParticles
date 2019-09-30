#pragma once

#include "GraphicsHeaders.h"
#include "RenderPass.h"

#include <stdexcept>

class Framebuffer {
public:
	Framebuffer(const VkDevice& logicalDevice, RenderPass*& renderPass, const VkImageView attachments[], const VkExtent2D& extent) : logicalDevice(logicalDevice) {
		VkFramebufferCreateInfo framebufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass->GetHandle(),
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = extent.width,
			.height = extent.height,
			.layers = 1
		};

		if (vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, nullptr, &framebufferHandle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan framebuffer.");
		}
	}

	~Framebuffer() {
		vkDestroyFramebuffer(logicalDevice, framebufferHandle, nullptr);
	}

	const VkFramebuffer& GetHandle() const {
		return framebufferHandle;
	}
private:
	VkFramebuffer framebufferHandle;
	const VkDevice& logicalDevice;
};

