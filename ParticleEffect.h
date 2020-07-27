#pragma once

#include "stdafx.h"
#include "Particle.h"
#include "VertexBuffer.h"

#include <vector>
#include <cstdlib>

class ParticleEffect
{
public:
	// ParticleEffect(std::shared_ptr<vk::Device>& logicalDevice,
	//                std::shared_ptr<vk::PhysicalDevice>& physicalDevice, vk::CommandPool& commandPool,
	//                vk::Queue& graphicsQueue, const glm::vec3 position, const uint32_t numParticles, const float particleSize)
	// 	: position_(position), numParticles_(numParticles),
	// 	  particleSize_(particleSize)
	// {
	// 	particles_.resize(numParticles_);
	// 	for (uint32_t i = 0; i < numParticles_; ++i)
	// 	{
	// 		auto magX = static_cast<float>(1 + rand() % 75) / 100.0f;
	// 		auto magZ = static_cast<float>(1 + rand() % 75) / 100.0f;
	// 		auto directionVector = glm::normalize(glm::vec3(magX, 1.0f, magZ));
	// 		particles_[i] = Particle(position, directionVector, glm::vec3(1.0f, 0.0f, 0.0f), particleSize_);
	// 	}
	// 	Update(0.0f);
	// }
	//
	// void Update(const float deltaTime)
	// {
	// 	std::vector<Vertex> allVertices;
	// 	for (auto particle : particles_)
	// 	{
	// 		particle.Update(deltaTime);
	// 		const auto& vertices = particle.GetVertices();
	// 		allVertices.insert(allVertices.end(), std::begin(vertices), std::end(vertices));
	// 	}
	// 	assert(allVertices.size() > 0);
	// 	meshData_.reset(new MeshData(parentLogicalDevice_, parentPhysicalDevice_, commandPool_, graphicsQueue_, allVertices));
	// }

private:
	// glm::vec3 position_;
	// uint32_t numParticles_;
	// float particleSize_;
	//
	// std::vector<Particle> particles_;
	// VertexBuffer vertexBuffer_;
};
