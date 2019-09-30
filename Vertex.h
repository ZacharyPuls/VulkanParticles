#pragma once

#include "GraphicsHeaders.h"

#include <array>

struct Vertex
{
	glm::vec3 Position;
	glm::vec4 Color;

	static VkVertexInputBindingDescription GetVertexInputBindingDescription() {
		return { .binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetVertexInputAttributeDescriptions() {
		return { { {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, Position) }, {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Vertex, Color) } } };
	}
};

