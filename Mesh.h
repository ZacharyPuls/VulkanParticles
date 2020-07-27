#pragma once

#include "VertexBuffer.h"
#include "Texture.h"
#include "IndexBuffer.h"

// TODO: Make this more intelligent. Add the ability to attach an arbitrary amount of render passes, shaders, effects, etc. to a mesh.
class Mesh
{
public:
	Mesh(const VulkanDeviceContext& deviceContext, const std::shared_ptr<vk::CommandPool>& commandPool,
	     const std::vector<Vertex>& vertices,
	     const std::vector<uint32_t> indices, const std::shared_ptr<Texture>& texture) : deviceContext_(deviceContext),
	                                                                                commandPool_(commandPool),
	                                                                                vertexBuffer_(
		                                                                                std::make_shared<
			                                                                                VertexBuffer>(
			                                                                                deviceContext_,
			                                                                                commandPool_,
			                                                                                vertices)),
	                                                                                indexBuffer_(
		                                                                                std::make_shared<
			                                                                                IndexBuffer>(
			                                                                                deviceContext_,
			                                                                                commandPool_,
			                                                                                indices)),
	                                                                                texture_(texture)
	{
	}

	~Mesh()
	{
		DebugMessage("Cleaning up VulkanParticles::Mesh.");
	}

	const std::shared_ptr<Texture>& GetTexture() const
	{
		return texture_;
	}

	void BindMeshData(const vk::CommandBuffer& commandBuffer) const
	{
		vertexBuffer_->Bind(commandBuffer);
		indexBuffer_->Bind(commandBuffer);
	}

	void Draw(const vk::CommandBuffer& commandBuffer) const
	{
		commandBuffer.drawIndexed(indexBuffer_->GetNumIndices(), 1, 0, 0, 0);
	}
private:
	const VulkanDeviceContext& deviceContext_;
	const std::shared_ptr<vk::CommandPool>& commandPool_;
	std::shared_ptr<VertexBuffer> vertexBuffer_;
	std::shared_ptr<IndexBuffer> indexBuffer_;
	const std::shared_ptr<Texture>& texture_;
};
