#pragma once

#include "GraphicsHeaders.h"

class Camera
{
public:
	Camera() : eye_(0.0f, 0.0f, 3.0f), direction_(0.0f, 0.0f, -1.0f), up_(0.0f, 1.0f, 0.0f), worldUp_(up_), yaw_(-90.0f), pitch_(0.0f), roll_(0.0f)
	{
		updateVectors_();
	}

	~Camera() = default;

	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(eye_, eye_ + direction_, up_);
	}

	void Forward(float deltaTime)
	{
		eye_ += direction_ * (SPEED * deltaTime);
	}

	void Backward(float deltaTime)
	{
		eye_ -= direction_ * (SPEED * deltaTime);
	}

	void Left(float deltaTime)
	{
		eye_ -= right_ * (SPEED * deltaTime);
	}

	void Right(float deltaTime)
	{
		eye_ += right_ * (SPEED * deltaTime);
	}

	void UpdateAngle(float deltaX, float deltaY)
	{
		deltaX *= SENSITIVITY;
		deltaY *= SENSITIVITY;

		yaw_ += deltaX;
		pitch_ += deltaY;

		pitch_ = (pitch_ > 89.0f) ? 89.0f : (pitch_ < -89.0f) ? -89.0f : pitch_;
		
		updateVectors_();
	}
private:
	glm::vec3 eye_;
	glm::vec3 direction_;
	glm::vec3 up_;

	glm::vec3 right_;
	glm::vec3 worldUp_;

	float fovY_;
	float aspect_;
	float zNear_;

	float yaw_;
	float pitch_;
	float roll_;

	static inline const float SENSITIVITY = 0.05f;
	static inline const float SPEED = 2.5f;

	void updateVectors_()
	{
		glm::vec3 target;
		target.x = glm::cos(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
		target.y = glm::sin(glm::radians(pitch_));
		target.z = glm::sin(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
		direction_ = glm::normalize(target);

		right_ = glm::normalize(glm::cross(direction_, worldUp_));
		up_ = glm::normalize(glm::cross(right_, direction_));
	}
};