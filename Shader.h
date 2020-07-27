#pragma once

#include "stdafx.h"

#include <vector>
#include "Vertex.h"

class Shader
{
public:
	Shader(const vk::UniqueDevice& logicalDevice_, const std::string& vertexShaderFilename,
	       const std::string& fragmentShaderFilename,
	       std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
	{
		auto vertexShaderSource = readFile_(vertexShaderFilename);
		auto fragmentShaderSource = readFile_(fragmentShaderFilename);

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		shaderc::SpvCompilationResult vertexShaderCompilationResult =
			compiler.CompileGlslToSpv(vertexShaderSource.str(), shaderc_glsl_vertex_shader, "vertex shader", options);
		if (vertexShaderCompilationResult.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << vertexShaderCompilationResult.GetErrorMessage();
		}
		auto vertexShaderCode = std::vector<uint32_t>{
			vertexShaderCompilationResult.cbegin(), vertexShaderCompilationResult.cend()
		};
		auto vertexShaderCodeSize = std::distance(vertexShaderCode.begin(), vertexShaderCode.end());
		auto vertexShaderCreateInfo =
			vk::ShaderModuleCreateInfo{{}, vertexShaderCodeSize * sizeof(uint32_t), vertexShaderCode.data()};
		vertexShaderModule_ = logicalDevice_->createShaderModuleUnique(vertexShaderCreateInfo);

		shaderc::SpvCompilationResult fragmentShaderCompilationResult = compiler.CompileGlslToSpv(
			fragmentShaderSource.str(), shaderc_glsl_fragment_shader, "fragment shader", options);
		if (fragmentShaderCompilationResult.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << fragmentShaderCompilationResult.GetErrorMessage();
		}
		auto fragmentShaderCode = std::vector<uint32_t>{
			fragmentShaderCompilationResult.cbegin(), fragmentShaderCompilationResult.cend()
		};
		auto fragmentShaderCodeSize = std::distance(fragmentShaderCode.begin(), fragmentShaderCode.end());
		auto fragmentShaderCreateInfo =
			vk::ShaderModuleCreateInfo{{}, fragmentShaderCodeSize * sizeof(uint32_t), fragmentShaderCode.data()};
		fragmentShaderModule_ = logicalDevice_->createShaderModuleUnique(fragmentShaderCreateInfo);

		auto vertShaderStageInfo = vk::PipelineShaderStageCreateInfo{
			{},
			vk::ShaderStageFlagBits::eVertex, *vertexShaderModule_, "main"
		};

		auto fragShaderStageInfo = vk::PipelineShaderStageCreateInfo{
			{},
			vk::ShaderStageFlagBits::eFragment, *fragmentShaderModule_, "main"
		};

		auto pipelineShaderStages =
			std::vector<vk::PipelineShaderStageCreateInfo>{vertShaderStageInfo, fragShaderStageInfo};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
			{}, static_cast<uint32_t>(descriptorSetLayoutBindings.size()), &descriptorSetLayoutBindings[0]);
		descriptorSetLayout_ = logicalDevice_->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);

		auto vertexInputBindingDescriptions = Vertex::GetVertexInputBindingDescription();
		auto vertexInputAttributeDescriptions = Vertex::GetVertexInputAttributeDescriptions();

		vertexInputInfo_ = vk::PipelineVertexInputStateCreateInfo{
			{}, 1, &vertexInputBindingDescriptions, vertexInputAttributeDescriptions.size(),
			&vertexInputAttributeDescriptions[0]
		};

		inputAssembly_ =
			vk::PipelineInputAssemblyStateCreateInfo{{}, vk::PrimitiveTopology::eTriangleList, false};
	}

private:
	vk::UniqueShaderModule vertexShaderModule_;
	vk::UniqueShaderModule fragmentShaderModule_;
	vk::UniqueDescriptorSetLayout descriptorSetLayout_;

	static std::stringstream readFile_(const std::string& filename)
	{
		std::ifstream file(filename);

		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file with filename [" + filename + "].");
		}

		std::stringstream buffer;
		buffer << file.rdbuf();

		file.close();

		return buffer;
	}
};
