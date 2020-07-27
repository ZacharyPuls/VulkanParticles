#pragma once

#include "stdafx.h"
#include "Vertex.h"

#include <array>

class Particle
{
public:


	Particle() = default;

	Particle(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& color, const float size)
		: position_(position),
		  velocity_(velocity),
		  color_(color),
		  size_(size),
		  life_(1.0f)
	{
	}

	void Update(const float deltaTime)
	{
		position_ += velocity_ * (DAMPENING * deltaTime);
		life_ -= (DAMPENING * deltaTime);
	}

	glm::vec3 GetPosition() const
	{
		return position_;
	}

	glm::vec4 GetColor() const
	{
		return glm::vec4(color_, life_);
	}

	std::array<Vertex, 6> GetVertices() const
	{
		const auto halfSize = size_ / 2.0f;
		return {
			Vertex(glm::vec3(position_.x - halfSize, position_.y - halfSize, position_.z), GetColor(), glm::vec2(0.0f, 0.0f)),
			Vertex(glm::vec3(position_.x + halfSize, position_.y - halfSize, position_.z), GetColor(), glm::vec2(1.0f, 0.0f)),
			Vertex(glm::vec3(position_.x - halfSize, position_.y + halfSize, position_.z), GetColor(), glm::vec2(0.0f, 1.0f)),
			Vertex(glm::vec3(position_.x + halfSize, position_.y - halfSize, position_.z), GetColor(), glm::vec2(1.0f, 0.0f)),
			Vertex(glm::vec3(position_.x + halfSize, position_.y + halfSize, position_.z), GetColor(), glm::vec2(1.0f, 1.0f)),
			Vertex(glm::vec3(position_.x - halfSize, position_.y + halfSize, position_.z), GetColor(), glm::vec2(0.0f, 1.0f))
		};
	}

private:
	glm::vec3 position_;
	glm::vec3 velocity_;
	glm::vec3 color_;
	float size_;
	float life_;
	static const inline float DAMPENING = 0.05f;
};
