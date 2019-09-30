#pragma once

#include "GraphicsHeaders.h"

#include <stdexcept>
#include <sstream>

class ShaderModule
{
public:
	ShaderModule(const VkDevice& logicalDevice, const std::stringstream& source) : logicalDevice(logicalDevice)
	{
		auto sourceString = source.str();
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = sourceString.length(),
			.pCode = reinterpret_cast<const uint32_t*>(&sourceString[0])
		};

		if (vkCreateShaderModule(logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModuleHandle) != VK_SUCCESS) {
			throw std::runtime_error("Could not create Vulkan shader module.");
		}
	}

	~ShaderModule()
	{
		vkDestroyShaderModule(logicalDevice, shaderModuleHandle, nullptr);
	}

	const VkShaderModule& GetHandle() const
	{
		return shaderModuleHandle;
	}
private:
	VkShaderModule shaderModuleHandle;
	const VkDevice& logicalDevice;
};
