#pragma once

#include "stdafx.h"

struct VulkanDeviceContext
{
	std::shared_ptr<vk::PhysicalDevice> PhysicalDevice;
	std::shared_ptr<vk::Device> LogicalDevice;
	std::shared_ptr<vk::Queue> GraphicsQueue;
	std::shared_ptr<vk::Queue> PresentQueue;
};
