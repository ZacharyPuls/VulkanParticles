#pragma once

#include <vulkan/vulkan.hpp>
#include "GLFW/glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

#define STBI_ONLY_JPEG
#include "stb/stb_image.h"