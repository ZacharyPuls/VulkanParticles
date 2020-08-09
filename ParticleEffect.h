#pragma once

#include "stdafx.h"
#include "Particle.h"
#include "Buffer.h"

#include <vector>
#include <cstdlib>

class ParticleEffect : public Mesh
{
public:
	// TODO: [zpuls 2020-07-27T20:30] dynamically figure out how large of a ::VertexBuffer to allocate, instead of using a hard-coded value of ::numParticles_ * 6
	/*ParticleEffect(const VulkanDeviceContext deviceContext, const std::shared_ptr<vk::CommandPool>& commandPool,
	               const glm::vec3 position, const uint32_t numParticles, const float particleSize,
	               const std::shared_ptr<Texture>& texture) : Mesh(deviceContext, commandPool, false, texture),
	                                                          position_(position), numParticles_(numParticles),
	                                                          particleSize_(particleSize),
	                                                          particles_(numParticles, Particle{}),
	                                                          stagingBuffer_(std::make_shared<VertexBuffer>(
		                                                          deviceContext_, commandPool_,
		                                                          numParticles_ * 6 * sizeof(Vertex),
		                                                          vk::BufferUsageFlagBits::eTransferSrc,
		                                                          vk::SharingMode::eExclusive,
		                                                          vk::MemoryPropertyFlagBits::eHostVisible |
		                                                          vk::MemoryPropertyFlagBits::eHostCoherent, "ParticleEffect::stagingBuffer_"))
	
	{
		
		Create(setupParticlesAndGenerateVertices_(position_, numParticles_, particleSize_), {});
	}*/


	ParticleEffect(const VulkanDeviceContext& deviceContext, vk::UniqueCommandPool& commandPool,
	               const uint32_t transformIndex, const uint32_t textureIndex,
	               const uint32_t descriptorSetIndex, const glm::vec3& position, const uint32_t numParticles,
	               const float particleSize, const uint32_t maxFramesInFlight) :
		Mesh(deviceContext, commandPool, false, transformIndex, textureIndex, descriptorSetIndex),
		position_(position), numParticles_(numParticles), particleSize_(particleSize),
		particles_(numParticles, Particle{}),
		stagingBuffer_(std::make_shared<VertexBuffer>(deviceContext_, commandPool_, numParticles_ * 6 * sizeof(Vertex),
		                                              vk::BufferUsageFlagBits::eTransferSrc,
		                                              vk::SharingMode::eExclusive,
		                                              vk::MemoryPropertyFlagBits::eHostVisible |
		                                              vk::MemoryPropertyFlagBits::eHostCoherent,
		                                              "ParticleEffect::stagingBuffer_"))
	{
		DebugMessage(
			"ParticleEffect::ParticleEffect(position={x=" + std::to_string(position_.x) + ",y=" +
			std::to_string(position_.y) + ",z=" + std::to_string(position_.z) + "},numParticles=" +
			std::to_string(numParticles_) + ",particleSize=" + std::to_string(particleSize_) + ")");
		Create(setupParticlesAndGenerateVertices_(position_, numParticles_, particleSize_), {}, maxFramesInFlight);
	}

	~ParticleEffect()
	{
		DebugMessage("ParticleEffect::~ParticleEffect()");
	}

	void Update(const float deltaTime)
	{
		const auto particleVertices = updateParticlesAndGenerateVertices_(deltaTime);
		auto vertexBufferSize = particleVertices.size() * sizeof(particleVertices[0]);
		stagingBuffer_->Fill(0, vertexBufferSize, &particleVertices[0]);
		stagingBuffer_->CopyTo(vertexBuffer_);
	}

private:
	glm::vec3 position_;
	uint32_t numParticles_;
	float particleSize_;
	std::vector<Particle> particles_;
	std::shared_ptr<VertexBuffer> stagingBuffer_;

	std::vector<Vertex> updateParticlesAndGenerateVertices_(const float deltaTime)
	{
		std::vector<Vertex> vertices;
		for (auto particle : particles_)
		{
			particle.Update(deltaTime);
			const auto& particleVertices = particle.GetVertices();
			vertices.insert(vertices.end(), std::begin(particleVertices), std::end(particleVertices));
		}

		return vertices;
	}

	std::vector<Vertex> setupParticlesAndGenerateVertices_(const glm::vec3 position, const uint32_t numParticles, const float particleSize)
	{
		for (uint32_t i = 0; i < numParticles; ++i)
		{
			auto magX = static_cast<float>(1 + rand() % 75) / 100.0f;
			auto magZ = static_cast<float>(1 + rand() % 75) / 100.0f;
			auto directionVector = glm::normalize(glm::vec3(magX, 1.0f, magZ));
			particles_[i] = Particle(position, directionVector, glm::vec3(1.0f, 0.0f, 0.0f), particleSize);
		}

		return updateParticlesAndGenerateVertices_(0.0f);
	}
};
