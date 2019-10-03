#pragma once

#include "GraphicsHeaders.h"
#include "ShaderModule.h"
#include "Vertex.h"
#include "PipelineLayout.h"
#include "RenderPass.h"

#include <stdexcept>
#include <sstream>
#include <string>

class GraphicsPipeline
{
public:
	GraphicsPipeline(const VkDevice& logicalDevice, const std::stringstream& vertexShaderSource, const std::stringstream& fragmentShaderSource, const std::string& name, const VkExtent2D& extent, RenderPass*& renderPass, const VkDescriptorSetLayout& descriptorSetLayout) : logicalDevice(logicalDevice), name(name)
	{
		ShaderModule vertexShaderModule(logicalDevice, vertexShaderSource);
		ShaderModule fragmentShaderModule(logicalDevice, fragmentShaderSource);

		VkPipelineShaderStageCreateInfo vertexShaderPipelineShaderStageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule.GetHandle(),
			.pName = name.c_str()
		};

		VkPipelineShaderStageCreateInfo fragmentShaderPipelineShaderStageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragmentShaderModule.GetHandle(),
			.pName = name.c_str()
		};

		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[] = { vertexShaderPipelineShaderStageCreateInfo, fragmentShaderPipelineShaderStageCreateInfo };

		auto vertexInputBindingDescription = Vertex::GetVertexInputBindingDescription();
		auto vertexInputAttributeDescriptions = Vertex::GetVertexInputAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &vertexInputBindingDescription,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size()),
			.pVertexAttributeDescriptions = &vertexInputAttributeDescriptions[0]
		};

		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		VkViewport viewport = {
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(extent.width),
			.height = static_cast<float>(extent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D scissor = {
			.offset = { 0, 0 },
			.extent = extent
		};

		VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		};

		VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f,
		};

		VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
		};

		VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &pipelineColorBlendAttachmentState,
			.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
		};

		pipelineLayout = new PipelineLayout(logicalDevice, descriptorSetLayout);

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = pipelineShaderStageCreateInfos,
			.pVertexInputState = &pipelineVertexInputStateCreateInfo,
			.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
			.pViewportState = &pipelineViewportStateCreateInfo,
			.pRasterizationState = &pipelineRasterizationStateCreateInfo,
			.pMultisampleState = &pipelineMultisampleStateCreateInfo,
			.pDepthStencilState = nullptr,
			.pColorBlendState = &pipelineColorBlendStateCreateInfo,
			.pDynamicState = nullptr,
			.layout = pipelineLayout->GetHandle(),
			.renderPass = renderPass->GetHandle(),
			.subpass = 0,
			.basePipelineHandle = nullptr,
			.basePipelineIndex = -1
		};

		if (vkCreateGraphicsPipelines(logicalDevice, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipelineHandle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan graphics pipeline.");
		}
	}

	~GraphicsPipeline()
	{
		vkDestroyPipeline(logicalDevice, pipelineHandle, nullptr);
		delete pipelineLayout;
	}

	const VkPipeline& GetHandle() const
	{
		return pipelineHandle;
	}
private:
	VkPipeline pipelineHandle;
	const VkDevice& logicalDevice;
	const std::string& name;
	PipelineLayout* pipelineLayout;
};
