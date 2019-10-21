#pragma once

#include "GraphicsHeaders.h"
#include "Vertex.h"
#include "Buffer.h"
#include "Texture.h"

#include <vector>
#include <unordered_map>

class Mesh
{
public:
	Mesh(std::shared_ptr<vk::Device> logicalDevice, std::shared_ptr<vk::PhysicalDevice> physicalDevice,
	     const vk::CommandPool& commandPool, const vk::Queue graphicsQueue,
	     const std::string& modelFilename, const std::string& textureFilename) :
		parentLogicalDevice_(logicalDevice), parentPhysicalDevice_(physicalDevice)
	{
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

		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex = {
					{
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					},
					{
						1.0f, 1.0f, 1.0f, 1.0f
					},
					{
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					}
				};
				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices_.size());
					vertices_.push_back(vertex);
				}
				indices_.push_back(uniqueVertices[vertex]);
			}
		}

		fillBuffer_(commandPool, graphicsQueue, reinterpret_cast<const void**>(&vertices_[0]), sizeof(vertices_[0]) * vertices_.size(), vertexBuffer_, vk::BufferUsageFlagBits::eVertexBuffer);
		fillBuffer_(commandPool, graphicsQueue, reinterpret_cast<const void**>(&indices_[0]), sizeof(indices_[0]) * indices_.size(), indexBuffer_, vk::BufferUsageFlagBits::eIndexBuffer);

		texture_.reset(new Texture(textureFilename, parentLogicalDevice_, parentPhysicalDevice_,
		                           commandPool, graphicsQueue, false));
	}

	Mesh(std::shared_ptr<vk::Device> logicalDevice, std::shared_ptr<vk::PhysicalDevice> physicalDevice,
		const vk::CommandPool& commandPool, const vk::Queue graphicsQueue,
		const std::vector<Vertex>& vertices, const std::vector<uint32_t> indices, const std::string& textureFilename) : parentLogicalDevice_(logicalDevice), parentPhysicalDevice_(physicalDevice), vertices_(vertices), indices_(indices)
	{
		
	}

	~Mesh() = default;

	const std::unique_ptr<Buffer>& GetVertexBuffer() const
	{
		return vertexBuffer_;
	}

	const std::unique_ptr<Buffer>& GetIndexBuffer() const
	{
		return indexBuffer_;
	}

	const vk::DeviceSize& GetNumIndices() const
	{
		return indices_.size();
	}

	const std::unique_ptr<Texture>& GetTexture() const
	{
		return texture_;
	}

private:
	std::shared_ptr<vk::Device> parentLogicalDevice_;
	std::shared_ptr<vk::PhysicalDevice> parentPhysicalDevice_;
	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;
	std::unique_ptr<Buffer> vertexBuffer_;
	std::unique_ptr<Buffer> indexBuffer_;
	std::unique_ptr<Texture> texture_;

	void fillBuffer_(const vk::CommandPool& commandPool, const vk::Queue graphicsQueue, const void** data, const vk::DeviceSize dataSize, std::unique_ptr<Buffer>& targetBuffer, vk::BufferUsageFlagBits usage)
	{
		auto stagingBuffer = std::make_unique<Buffer>(parentLogicalDevice_, parentPhysicalDevice_, dataSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::SharingMode::eExclusive,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::
			MemoryPropertyFlagBits::eHostCoherent);
		stagingBuffer->Fill(0, dataSize, &data[0]);
		targetBuffer.reset(new Buffer(parentLogicalDevice_, parentPhysicalDevice_, dataSize,
			vk::BufferUsageFlagBits::eTransferDst | usage,
			{}, vk::MemoryPropertyFlagBits::eDeviceLocal));
		Buffer::Copy(stagingBuffer, targetBuffer, dataSize, commandPool, graphicsQueue);
	}
};
