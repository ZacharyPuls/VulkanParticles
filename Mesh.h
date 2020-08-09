#pragma once

#include "Texture.h"
#include "Buffer.h"

// TODO: Make this more intelligent. Add the ability to attach an arbitrary amount of render passes, shaders, effects, etc. to a mesh.
class Mesh
{
public:
	Mesh(const VulkanDeviceContext deviceContext, vk::UniqueCommandPool& commandPool,
	     const bool isIndexed, const uint32_t transformIndex, const uint32_t textureIndex,
	     const uint32_t descriptorSetIndex) : deviceContext_(deviceContext), commandPool_(commandPool),
	                                          isIndexed_(isIndexed), transformIndex_(transformIndex),
	                                          textureIndex_(textureIndex), descriptorSetIndex_(descriptorSetIndex)
	{
		DebugMessage("Mesh::Mesh()");
	}

	~Mesh()
	{
		DebugMessage("Cleaning up VulkanParticles::Mesh.");
	}

	void Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t> indices, const uint32_t maxFramesInFlight)
	{
		DebugMessage("Mesh::Create()");
		auto vertexBufferSize = vertices.size() * sizeof(vertices[0]);
		auto vertexStagingBuffer = std::make_shared<VertexBuffer>(deviceContext_, commandPool_, vertexBufferSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::SharingMode::eExclusive,
			vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent, "Mesh::vertexStagingBuffer");
		vertexStagingBuffer->Fill(0, vertexBufferSize, &vertices[0]);

		vertexBuffer_ = std::make_shared<VertexBuffer>(deviceContext_, commandPool_,
			vertexBufferSize,
			vk::BufferUsageFlagBits::eTransferDst |
			vk::BufferUsageFlagBits::eVertexBuffer,
			vk::SharingMode::eExclusive,
			vk::MemoryPropertyFlagBits::eDeviceLocal, "Mesh::vertexBuffer_");

		vertexStagingBuffer->CopyTo(vertexBuffer_);

		if (isIndexed_)
		{
			auto indexBufferSize = indices.size() * sizeof(indices[0]);
			auto indexStagingBuffer = std::make_shared<IndexBuffer>(deviceContext_, commandPool_, indexBufferSize,
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::SharingMode::eExclusive,
				vk::MemoryPropertyFlagBits::eHostVisible |
				vk::MemoryPropertyFlagBits::eHostCoherent, "Mesh::indexStagingBuffer");

			indexStagingBuffer->Fill(0, indexBufferSize, &indices[0]);

			indexBuffer_ = std::make_shared<IndexBuffer>(deviceContext_, commandPool_, indexBufferSize,
				vk::BufferUsageFlagBits::eTransferDst |
				vk::BufferUsageFlagBits::eIndexBuffer,
				vk::SharingMode::eExclusive,
				vk::MemoryPropertyFlagBits::eDeviceLocal, "Mesh::indexBuffer_");

			indexStagingBuffer->CopyTo(indexBuffer_);
		}
		else
		{
			indexBuffer_ = nullptr;
		}

		// TODO: [zpuls 2020-08-04T17:49] Handle Uniform Buffer creation better, allow for dynamic uniform binding based on bound shader.
		uniformBuffers_ = std::vector<std::shared_ptr<GenericBuffer>>(maxFramesInFlight,
		                                                              std::make_shared<GenericBuffer>(
			                                                              deviceContext_, commandPool_,
			                                                              sizeof(glm::mat4) * 3,
			                                                              vk::BufferUsageFlagBits::eUniformBuffer,
			                                                              vk::SharingMode::eExclusive,
			                                                              vk::MemoryPropertyFlagBits::eHostVisible |
			                                                              vk::MemoryPropertyFlagBits::eHostCoherent,
			                                                              "Mesh::uniformBuffers_"));

		// TODO: [zpuls 2020-08-02T22:02] Create DescriptorSetLayout/DescriptorSets based on the Mesh-bound shaders active at runtime.	
	}

	// TODO: [zpuls 2020-08-02T21:56] Allow for multiple vk::DescriptorSet bindings per-mesh.
	void BindMeshData(const uint32_t frameIndex, vk::UniqueCommandBuffer& commandBuffer, std::array<glm::mat4, 3> mvp) const
	{
		DebugMessage("Mesh::BindMeshData()");
		vertexBuffer_->Bind(commandBuffer);
		if (isIndexed_)
		{
			indexBuffer_->Bind(commandBuffer);
		}
		uniformBuffers_[frameIndex]->Fill(0, sizeof(mvp), &mvp[0]);
	}

	void Draw(vk::UniqueCommandBuffer& commandBuffer) const
	{
		DebugMessage("Mesh::Draw()");
		if (isIndexed_)
		{
			commandBuffer->drawIndexed(indexBuffer_->GetNumElements(), 1, 0, 0, 0);
		}
		else
		{
			commandBuffer->draw(vertexBuffer_->GetNumElements(), 1, 0, 0);
		}
	}

	void Update(const float deltaTime)
	{
		
	}

	[[nodiscard]] uint32_t GetTextureIndex() const
	{
		return textureIndex_;
	}

	[[nodiscard]] uint32_t GetTransformIndex() const
	{
		return transformIndex_;
	}

	[[nodiscard]] uint32_t GetDescriptorSetIndex() const
	{
		return descriptorSetIndex_;
	}

	[[nodiscard]] const std::shared_ptr<GenericBuffer>& GetUniformBuffer(const uint32_t index) const
	{
		return uniformBuffers_[index];
	}

protected:
	VulkanDeviceContext deviceContext_;
	vk::UniqueCommandPool& commandPool_;
	std::shared_ptr<VertexBuffer> vertexBuffer_;
	bool isIndexed_;
	std::shared_ptr<IndexBuffer> indexBuffer_;
	std::vector<std::shared_ptr<GenericBuffer>> uniformBuffers_;
	uint32_t textureIndex_;
	uint32_t transformIndex_;
	uint32_t descriptorSetIndex_;
};
