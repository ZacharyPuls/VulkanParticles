#pragma once

#include "stdafx.h"

struct Transform
{
	Transform() = default;

	Transform(const glm::vec3& translation, const glm::vec3& rotationAxis, const float rotationAngle,
	          const glm::vec3& scale)
		: Translation(translation),
		  RotationAxis(rotationAxis),
		  RotationAngle(rotationAngle),
		  Scale(scale)
	{
	}

	Transform(const Transform& other)
		: Translation(other.Translation),
		  RotationAxis(other.RotationAxis),
		  RotationAngle(other.RotationAngle),
		  Scale(other.Scale)
	{
	}

	Transform(Transform&& other) noexcept
		: Translation(std::move(other.Translation)),
		  RotationAxis(std::move(other.RotationAxis)),
		  RotationAngle(other.RotationAngle),
		  Scale(std::move(other.Scale))
	{
	}

	Transform& operator=(const Transform& other)
	{
		if (this == &other)
			return *this;
		Translation = other.Translation;
		RotationAxis = other.RotationAxis;
		RotationAngle = other.RotationAngle;
		Scale = other.Scale;
		return *this;
	}

	Transform& operator=(Transform&& other) noexcept
	{
		if (this == &other)
			return *this;
		Translation = std::move(other.Translation);
		RotationAxis = std::move(other.RotationAxis);
		RotationAngle = other.RotationAngle;
		Scale = std::move(other.Scale);
		return *this;
	}

	glm::vec3 Translation;
	glm::vec3 RotationAxis;
	float RotationAngle;
	glm::vec3 Scale;

	glm::mat4 GetModelMatrix()
	{
		return glm::translate(glm::mat4(1.0f), Translation) * glm::rotate(glm::mat4(1.0f), RotationAngle, RotationAxis) * glm::scale(glm::mat4(1.0f), Scale);
	}
};
