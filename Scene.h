#pragma once

#include "stdafx.h"

#include <vector>
#include "Mesh.h"

class Scene
{
public:
	Scene(const VulkanDeviceContext& deviceContext, const std::shared_ptr<vk::CommandPool>& vkCommandPool)
		: deviceContext_(deviceContext),
		  commandPool_(vkCommandPool)
	{
	}

	~Scene()
	{
		DebugMessage("Cleaning up VulkanParticles::Scene.");
	}

	const std::shared_ptr<Mesh>& LoadObj(const std::string& modelFilename, const std::string& textureFilename)
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
				vertices.push_back({
					{
						attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					},
					{
						1.0f, 1.0f, 1.0f, 1.0f
					},
					{
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					}
					});
				indices.push_back(indices.size());
			}
		}

		const auto& texture = LoadTexture(textureFilename);
		meshes_.push_back(std::make_shared<Mesh>(deviceContext_, commandPool_, vertices, indices, texture));
		return meshes_.back();
	}

	const std::shared_ptr<Texture>& LoadTexture(const std::string& filename)
	{
		textures_.push_back(std::make_shared<Texture>(filename, deviceContext_, commandPool_, true));
		return textures_.back();
	}
private:
	const VulkanDeviceContext deviceContext_;
	const std::shared_ptr<vk::CommandPool>& commandPool_;
	std::vector<std::shared_ptr<Mesh>> meshes_;
	std::vector<std::shared_ptr<Texture>> textures_;
	
};
