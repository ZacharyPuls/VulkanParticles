#pragma once

#include "GraphicsHeaders.h"

#include <array>

#pragma pack(push, 1)
struct Vertex
{
	glm::vec3 Position;
	glm::vec4 Color;

	static vk::VertexInputBindingDescription GetVertexInputBindingDescription() {
		return vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
	}

	static std::array<vk::VertexInputAttributeDescription, 2> GetVertexInputAttributeDescriptions() {
		return {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Position)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, Color))
		};
	}
};
#pragma pack(pop)