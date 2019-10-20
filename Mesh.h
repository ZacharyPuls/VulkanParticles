#pragma once

#include "GraphicsHeaders.h"
#include "Vertex.h"
#include "Buffer.h"
#include "Texture.h"

#include <vector>

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

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				vertices_.push_back({
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
				indices_.push_back(indices_.size());
			}
		}

		auto bufferSize = sizeof(vertices_[0]) * vertices_.size();

		auto stagingBuffer = std::make_unique<Buffer>(parentLogicalDevice_, parentPhysicalDevice_, bufferSize,
		                                              vk::BufferUsageFlagBits::eTransferSrc,
		                                              vk::SharingMode::eExclusive,
		                                              vk::MemoryPropertyFlagBits::eHostVisible | vk::
		                                              MemoryPropertyFlagBits::eHostCoherent);
		stagingBuffer->Fill(0, bufferSize, &vertices_[0]);
		vertexBuffer_.reset(new Buffer(parentLogicalDevice_, parentPhysicalDevice_, bufferSize,
		                               vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		                               {}, vk::MemoryPropertyFlagBits::eDeviceLocal));
		Buffer::Copy(stagingBuffer, vertexBuffer_, bufferSize, commandPool, graphicsQueue);

		bufferSize = sizeof(indices_[0]) * indices_.size();
		stagingBuffer.reset(new Buffer(parentLogicalDevice_, parentPhysicalDevice_, bufferSize,
		                               vk::BufferUsageFlagBits::eTransferSrc,
		                               vk::SharingMode::eExclusive,
		                               vk::MemoryPropertyFlagBits::eHostVisible | vk::
		                               MemoryPropertyFlagBits::eHostCoherent));
		stagingBuffer->Fill(0, bufferSize, &indices_[0]);
		indexBuffer_.reset(new Buffer(parentLogicalDevice_, parentPhysicalDevice_, bufferSize,
		                              vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, {},
		                              vk::MemoryPropertyFlagBits::eDeviceLocal));
		Buffer::Copy(stagingBuffer, indexBuffer_, bufferSize, commandPool, graphicsQueue);

		texture_.reset(new Texture(textureFilename, parentLogicalDevice_, parentPhysicalDevice_,
		                           commandPool, graphicsQueue));
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
};
