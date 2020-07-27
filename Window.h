#pragma once

#include "stdafx.h"

#include <string>
#include <cstdint>
#include <vector>
#include <array>

class Window
{
public:

	explicit Window(const std::string& title, const int width, const int height)
		: title_(title)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		windowHandle_ = glfwCreateWindow(width, height, title_.c_str(), nullptr, nullptr);
		glfwSetKeyCallback(windowHandle_, keyboardCallback_);
	}

	~Window()
	{
		glfwDestroyWindow(windowHandle_);
		glfwTerminate();
	}
	
	std::vector<const char *> GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		requiredExtensions.push_back("VK_EXT_debug_utils");
		return requiredExtensions;
	}

	std::string GetTitle() const
	{
		return title_;
	}

	GLFWwindow* GetHandle() const
	{
		return windowHandle_;
	}

	std::array<int, 2> GetDimensions() const
	{
		int width = 0;
		int height = 0;
		glfwGetWindowSize(windowHandle_, &width, &height);
		return { width, height };
	}
	
private:
	const std::string title_;
	GLFWwindow* windowHandle_;

	static void keyboardCallback_(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		
	}
};