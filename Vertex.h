#pragma once

#include "stdafx.h"

#include <array>

#pragma pack(push, 1)
struct Vertex
{
	glm::vec3 Position;
	glm::vec4 Color;
	glm::vec2 TexCoord;


	Vertex(const glm::vec3& position, const glm::vec4& color, const glm::vec2& texCoord)
		: Position(position),
		  Color(color),
		  TexCoord(texCoord)
	{
	}

	Vertex(const Vertex& other)
		: Position(other.Position),
		  Color(other.Color),
		  TexCoord(other.TexCoord)
	{
	}

	Vertex(Vertex&& other) noexcept
		: Position(std::move(other.Position)),
		  Color(std::move(other.Color)),
		  TexCoord(std::move(other.TexCoord))
	{
	}

	Vertex& operator=(const Vertex& other)
	{
		if (this == &other)
			return *this;
		Position = other.Position;
		Color = other.Color;
		TexCoord = other.TexCoord;
		return *this;
	}

	Vertex& operator=(Vertex&& other) noexcept
	{
		if (this == &other)
			return *this;
		Position = std::move(other.Position);
		Color = std::move(other.Color);
		TexCoord = std::move(other.TexCoord);
		return *this;
	}

	static vk::VertexInputBindingDescription GetVertexInputBindingDescription()
	{
		return vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
	}

	static std::array<vk::VertexInputAttributeDescription, 3> GetVertexInputAttributeDescriptions()
	{
		return {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Position)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, Color)),
			vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, TexCoord))
		};
	}


	friend bool operator==(const Vertex& lhs, const Vertex& rhs)
	{
		return lhs.Position == rhs.Position
			&& lhs.Color == rhs.Color
			&& lhs.TexCoord == rhs.TexCoord;
	}

	friend bool operator!=(const Vertex& lhs, const Vertex& rhs)
	{
		return !(lhs == rhs);
	}
};
#pragma pack(pop)

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.Position) ^
				(hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}