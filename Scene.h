#pragma once

#include "stdafx.h"

#include <vector>
#include <glm/gtx/string_cast.hpp>


#include "DescriptorSet.h"
#include "Mesh.h"
#include "Transform.h"

class Scene
{
public:
	Scene(const uint32_t maxFramesInFlight) : maxFramesInFlight_(maxFramesInFlight)
	{
		auto cameraIndex = AddCamera();
		SetActiveCamera(cameraIndex);
	}

	~Scene()
	{
		DebugMessage("Cleaning up VulkanParticles::Scene.");
	}

	const uint32_t LoadObj(const std::string& modelFilename, const std::string& textureFilename, std::shared_ptr<VulkanContext> vulkanContext, const uint32_t descriptorSetIndex)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warning;
		std::string error;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, modelFilename.c_str()))
		{
			throw std::runtime_error(
				"Could not load model file [" + modelFilename + "], warning [" + warning + "] | error [" + error +
				"].");
		}

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				vertices.emplace_back(Vertex{
					{
						attrib.vertices[3 * static_cast<uint32_t>(index.vertex_index) + 0], attrib.vertices[3 * static_cast<uint32_t>(index.vertex_index) + 1],
						attrib.vertices[3 * static_cast<uint32_t>(index.vertex_index) + 2]
					},
					{
						1.0f, 1.0f, 1.0f, 1.0f
					},
					{
						attrib.texcoords[2 * static_cast<uint32_t>(index.texcoord_index) + 0],
						1.0f - attrib.texcoords[2 * static_cast<uint32_t>(index.texcoord_index) + 1]
					}
					});
				indices.emplace_back(static_cast<uint32_t>(indices.size()));
			}
		}

		const auto textureIndex = AddTexture(textureFilename, vulkanContext);
		const auto transformIndex = AddTransform({}, glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(-90.0f), glm::vec3(1.0f));
		return AddMesh(textureIndex, transformIndex, vertices, indices, vulkanContext, descriptorSetIndex);
	}

	uint32_t AddParticleEffect(const uint32_t textureIndex, const uint32_t transformIndex, const glm::vec3& position, const uint32_t numParticles, const float particleSize, std::shared_ptr<VulkanContext> vulkanContext, const uint32_t descriptorSetIndex)
	{
		DebugMessage("Scene::AddParticleEffect()");
		// const auto transformIndex = AddTransform(glm::vec3(), glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(-90.0f), glm::vec3(1.0f));
		// const auto textureIndex = AddTexture(textureFilename);
		// TODO: [zpuls 2020-08-04T17:33] Generate this dynamically based on what shader the user wants to attach to the mesh, instead of hard-coding it
		// const auto descriptorSetLayoutIndex = AddDescriptorSetLayout({
		// 	{1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		// 	{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}
		// 	});
		// const auto descriptorSetIndex = AddDescriptorSet(descriptorSetLayoutIndex);
		const auto& particleEffect = std::make_shared<ParticleEffect>(vulkanContext->GetDeviceContext(),
		                                                              vulkanContext->GetCommandPool(), transformIndex,
		                                                              textureIndex, descriptorSetIndex, position,
		                                                              numParticles, particleSize, maxFramesInFlight_);
		particleEffects_.emplace_back(particleEffect);
		return particleEffects_.size() - 1;
	}

	uint32_t AddMesh(const uint32_t textureIndex, const uint32_t transformIndex, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, std::shared_ptr<VulkanContext> vulkanContext, const uint32_t descriptorSetIndex)
	{
		DebugMessage("Scene::AddMesh()");
		// TODO: [zpuls 2020-08-03T23:19] Generate this dynamically based on what shader the user wants to attach to the mesh, instead of hard-coding it
		// const auto descriptorSetLayoutIndex = AddDescriptorSetLayout({
		// 	{1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		// 	{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}
		// });
		// const auto descriptorSetIndex = AddDescriptorSet(descriptorSetLayoutIndex);
		const auto& mesh = std::make_shared<Mesh>(vulkanContext->GetDeviceContext(), vulkanContext->GetCommandPool(),
		                                          true, transformIndex, textureIndex, descriptorSetIndex);
		mesh->Create(vertices, indices, maxFramesInFlight_);
		meshes_.emplace_back(mesh);
		return meshes_.size() - 1;
	}

	std::shared_ptr<Mesh> GetMesh(const uint32_t index)
	{
		return meshes_[index];
	}

	std::shared_ptr<ParticleEffect> GetParticleEffect(const uint32_t index)
	{
		return particleEffects_[index];
	}
	
	uint32_t AddTexture(const std::string& filename, std::shared_ptr<VulkanContext> vulkanContext)
	{
		textures_.emplace_back(std::make_shared<Texture>(filename, vulkanContext->GetDeviceContext(),
		                                                 vulkanContext->GetCommandPool(), true));
		return textures_.size() - 1;
	}

	std::shared_ptr<Texture> GetTexture(const uint32_t index)
	{
		return textures_[index];
	}

	uint32_t AddTransform(const glm::vec3& translation, const glm::vec3& rotationAxis, const float rotationAngle,
	                      const glm::vec3& scale)
	{
		transforms_.emplace_back(std::make_shared<Transform>(translation, rotationAxis, rotationAngle, scale));
		return transforms_.size() - 1;
	}

	std::shared_ptr<Transform> GetTransform(const uint32_t index)
	{
		return transforms_[index];
	}

	// TODO: [zpuls 2020-08-04T16:51] Break rendering functionality out into a separate rendering object

	std::array<glm::mat4, 3> GetMVP(std::shared_ptr<Mesh> mesh)
	{
		auto modelMatrix = transforms_[mesh->GetTransformIndex()]->GetModelMatrix();
		auto viewMatrix = cameras_[activeCamera_]->GetViewMatrix();
		DebugMessage("Scene::GetMVP() - M: " + glm::to_string(modelMatrix) + ", V: " + glm::to_string(viewMatrix) + ", P: " + glm::to_string(projectionMatrix_));
		return {
			modelMatrix,
			viewMatrix,
			projectionMatrix_
		};
	}

	std::array<glm::mat4, 3> GetMVP(std::shared_ptr<ParticleEffect> particleEffect)
	{
		auto modelMatrix = transforms_[particleEffect->GetTransformIndex()]->GetModelMatrix();
		auto viewMatrix = cameras_[activeCamera_]->GetViewMatrix();
		DebugMessage("Scene::GetMVP() - M: " + glm::to_string(modelMatrix) + ", V: " + glm::to_string(viewMatrix) + ", P: " + glm::to_string(projectionMatrix_));
		return {
			modelMatrix,
			viewMatrix,
			projectionMatrix_
		};
	}

	// TODO: [zpuls 2020-08-04T17:42] Allow for adding different types of cameras, selecting from a list of types (freecam, fixed, follow, arcball, etc), and allow passing in initial position/transform parameters.
	uint32_t AddCamera()
	{
		cameras_.emplace_back(std::make_shared<Camera>());
		return cameras_.size() - 1;
	}

	void SetActiveCamera(const uint32_t cameraIndex)
	{
		activeCamera_ = cameraIndex;
	}

	std::shared_ptr<Camera> GetActiveCamera()
	{
		return cameras_[activeCamera_];
	}

	void SetSwapchainExtent(const vk::Extent2D extent)
	{
		projectionMatrix_ = glm::perspective(glm::radians(45.0f), extent.width / static_cast<float>(extent.height),
		                                     0.1f, 10.0f);

		projectionMatrix_[1][1] *= -1.0f;
		DebugMessage("Scene::SetSwapchainExtent({" + std::to_string(extent.width) + "," + std::to_string(extent.height) + "}) - " + glm::to_string(projectionMatrix_));
	}

	[[nodiscard]] std::vector<std::shared_ptr<Mesh>> GetMeshes() const
	{
		return meshes_;
	}

	[[nodiscard]] std::vector<std::shared_ptr<ParticleEffect>> GetParticleEffects() const
	{
		return particleEffects_;
	}
private:
	const uint32_t maxFramesInFlight_;
	// std::vector<std::shared_ptr<Buffer>> buffers_;
	std::vector<std::shared_ptr<Mesh>> meshes_;
	std::vector<std::shared_ptr<ParticleEffect>> particleEffects_;
	std::vector<std::shared_ptr<Texture>> textures_;
	std::vector<std::shared_ptr<Camera>> cameras_;
	std::vector<std::shared_ptr<Camera>>::size_type activeCamera_;
	glm::mat4 projectionMatrix_;
	std::vector<std::shared_ptr<Transform>> transforms_;
};
