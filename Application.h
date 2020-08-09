#pragma once

#include "stdafx.h"
#include "Vertex.h"
#include "Buffer.h"
#include "Texture.h"
#include "Mesh.h"
#include "Camera.h"
#include "ParticleEffect.h"
#include "VulkanContext.h"
#include "Scene.h"

#include <glm/gtx/string_cast.hpp>

#include "VulkanRenderingEngine.h"

const std::vector<const char*> VK_VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
	// , "VK_LAYER_LUNARG_api_dump"
};

class Application
{
public:
	Application()
	{
		InitializeGlfw();
		CreateGlfwWindow();
		CreateVulkanContext();
		
		int width, height;
		glfwGetFramebufferSize(appWindow_, &width, &height);

		const auto& vertexShaderSource = ReadFile("assets/shaders/vert.spv");
		const auto& fragmentShaderSource = ReadFile("assets/shaders/frag.spv");

		context_->Initialize(appWindow_, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
		                     vk::Format::eB8G8R8A8Unorm, vertexShaderSource, fragmentShaderSource, VIEWPORT_MIN_DEPTH,
		                     VIEWPORT_MAX_DEPTH, MAX_FRAMES_IN_FLIGHT);

		scene_ = std::make_shared<Scene>(MAX_FRAMES_IN_FLIGHT);
		scene_->SetSwapchainExtent(context_->GetSwapchainExtent());

		// TODO: [zpuls 2020-08-08T18:07] Allow for dynamic descriptor set layouts based on shader/material, not just relying on the default one created during VulkanContext::Initialize().
		const auto defaultMeshDescriptorSetLayoutIndex = 0;
		
		// auto chaletMeshDescriptorSet = context_->AddDescriptorSet(defaultMeshDescriptorSetLayoutIndex);
		
		// chaletMesh_ = scene_->LoadObj("assets/models/chalet/chalet.obj", "assets/models/chalet/chalet.jpg", context_, chaletMeshDescriptorSet);
		
		auto particleEffectDescriptorSet = context_->AddDescriptorSet(defaultMeshDescriptorSetLayoutIndex);
		
		auto particleEffectTexture = scene_->AddTexture("assets/textures/particle/texture.png", context_);
		// TODO: [zpuls 2020-08-08T20:53] More error-handling with Transform, allow the user to add no rotation/no translation/no scale, etc, without getting -NaN values in the resulting glm::mat4 (Model Matrix)
		auto particleEffectTransform = scene_->AddTransform(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 0.0f, glm::vec3(1.0f));
 		particleEffect_ = scene_->AddParticleEffect(particleEffectTexture, particleEffectTransform, glm::vec3(0.0f, 0.0f, 1.0f), 5, 0.1f, context_, particleEffectDescriptorSet);

		renderingEngine_ = std::make_shared<VulkanRenderingEngine>(context_, MAX_FRAMES_IN_FLIGHT);

		renderingEngine_->UpdateScene(scene_);
	}

	~Application()
	{
		DebugMessage("Application::~Application()");
		CleanupVulkan();
		glfwDestroyWindow(appWindow_);
		glfwTerminate();
	}

	void Run()
	{
		MainLoop();
	}

private:
	GLFWwindow* appWindow_;
	std::shared_ptr<VulkanContext> context_;

	std::shared_ptr<Scene> scene_;
	std::shared_ptr<VulkanRenderingEngine> renderingEngine_;
	
	uint32_t chaletMesh_;
	uint32_t particleEffect_;
	
	size_t currentFrame = 0;
	bool framebufferResized = false;

	float deltaTime_; // time between rendering frames, used for framerate-independent camera movement

	const int DEFAULT_WINDOW_WIDTH = 1366;
	const int DEFAULT_WINDOW_HEIGHT = 1024;
	inline static const char* APP_NAME = "Vulkan Particles!";
	inline static const uint32_t APP_VERSION = VK_MAKE_VERSION(0, 0, 1);
	inline static const char* ENGINE_NAME = "BZEngine";
	inline static const uint32_t ENGINE_VERSION = VK_MAKE_VERSION(0, 0, 1);
	inline static const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	inline static const float VIEWPORT_MIN_DEPTH = 0.0f;
	inline static const float VIEWPORT_MAX_DEPTH = 1.0f;
	inline static const int MAX_FRAMES_IN_FLIGHT = 3;

	struct ModelViewProj
	{
		alignas(16) glm::mat4 Model;
		alignas(16) glm::mat4 View;
		alignas(16) glm::mat4 Proj;
	};

	void InitializeGlfw()
	{
		if (!glfwInit())
		{
			throw std::runtime_error("Could not initialize GLFW.");
		}

		if (!glfwVulkanSupported())
		{
			throw std::runtime_error("Vulkan is not supported on this platform.");
		}

		glfwSetErrorCallback(ThrowGlfwErrorAsException);
	}

	void CreateGlfwWindow()
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		appWindow_ = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, APP_NAME, nullptr, nullptr);

		glfwSetWindowUserPointer(appWindow_, this);
		glfwSetKeyCallback(appWindow_, KeyboardCallback);
		glfwSetFramebufferSizeCallback(appWindow_, ResizeCallback);
		glfwSetCursorPosCallback(appWindow_, MouseCallback);
		glfwSetInputMode(appWindow_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		if (!appWindow_)
		{
			throw std::runtime_error("Could not create app window.");
		}
	}

	static std::vector<const char*> getGlfwRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
	}

	void CreateVulkanContext()
	{
		const uint32_t enabledLayerCount = ENABLE_VK_VALIDATION_LAYERS ? 1 : 0;
		std::vector<const char*> enabledLayers;

		if (ENABLE_VK_VALIDATION_LAYERS) {
			enabledLayers = VK_VALIDATION_LAYERS;
		}
		
		auto requiredGlfwExtensions = getGlfwRequiredExtensions();
		context_ = std::make_shared<VulkanContext>(enabledLayers, requiredGlfwExtensions, APP_NAME, APP_VERSION, ENGINE_NAME, ENGINE_VERSION);
	}

	void CreateVulkanSurface()
	{
		context_->CreateSurface(appWindow_);
	}

	void SelectVulkanPhysicalDevice()
	{
		context_->SelectPhysicalDevice();
	}

	void CreateVulkanLogicalDevice()
	{
		context_->CreateLogicalDevice();
	}

	void CreateVulkanSwapchain()
	{
		int width, height;
		glfwGetFramebufferSize(appWindow_, &width, &height);

		VkExtent2D requestedSwapchainExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		
		context_->CreateSwapchain(requestedSwapchainExtent);
	}

	void CreateVulkanImageViews(const vk::Format imageFormat)
	{
		context_->CreateImageViews(imageFormat);
	}

	void CreateVulkanRenderPass(const vk::Format imageFormat)
	{
		context_->CreateRenderPass(imageFormat);
	}

	static std::stringstream ReadFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file with filename [" + filename + "].");
		}

		std::stringstream buffer;
		buffer << file.rdbuf();

		file.close();

		return buffer;
	}
	
	static void ThrowGlfwErrorAsException(int error, const char* description)
	{
		const auto exceptionMessage = "GLFW encountered an error with code [" + std::to_string(error) +
			"], and description [" + std::string(description) + "].";
		throw std::runtime_error(exceptionMessage.c_str());
	}

	static void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}

	static void ResizeCallback(GLFWwindow* window, int width, int height)
	{
		const auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	static void MouseCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		
		static double lastX = app->DEFAULT_WINDOW_WIDTH / 2.0f;
		static double lastY = app->DEFAULT_WINDOW_HEIGHT / 2.0f;

		float deltaX = xpos - lastX;
		float deltaY = lastY - ypos;
		lastX = xpos;
		lastY = ypos;

		app->scene_->GetActiveCamera()->UpdateAngle(deltaX, deltaY);
		// app->camera_->UpdateAngle(deltaX, deltaY);	
	}

	void CreateVulkanDescriptorPool()
	{
		context_->CreateDescriptorPool();
	}

	vk::Extent2D GetWindowSize()
	{
		auto width = 0, height = 0;
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(appWindow_, &width, &height);
			glfwWaitEvents();
		}

		return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	}

	void RecreateVulkanSwapchain()
	{
		context_->RecreateSwapchain(GetWindowSize());

		// CleanupVulkanSwapchain();
		//
		// CreateVulkanSwapchain();
		// CreateVulkanImageViews();
		// CreateVulkanRenderPass();
		// CreateVulkanGraphicsPipeline();
		// CreateVulkanDepthBuffer();
		// CreateVulkanFramebuffers();
		// CreateVulkanUniformBuffers();
		// CreateVulkanDescriptorPool();
		// CreateVulkanDescriptorSets();
		// CreateVulkanCommandBuffers();
	}

	void RenderFrame()
	{
		auto currentWindowSize = GetWindowSize();
		auto imageIndex = renderingEngine_->BeginFrame(currentWindowSize);
		renderingEngine_->RenderScene(scene_, imageIndex, currentWindowSize, deltaTime_);
		renderingEngine_->EndFrame();
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(appWindow_))
		{
			// static auto startTime = std::chrono::high_resolution_clock::now();

			// auto currentTime = std::chrono::high_resolution_clock::now();
			// deltaTime_ = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

			// scene_->Update(currentFrame);
			
			static auto lastTime = glfwGetTime();
			auto currentTime = glfwGetTime();
			deltaTime_ = currentTime - lastTime;
			lastTime = currentTime;
			
			if (glfwGetKey(appWindow_, GLFW_KEY_W) == GLFW_PRESS)
			{
				scene_->GetActiveCamera()->Forward(deltaTime_);
			}

			if (glfwGetKey(appWindow_, GLFW_KEY_A) == GLFW_PRESS)
			{
				scene_->GetActiveCamera()->Left(deltaTime_);
			}

			if (glfwGetKey(appWindow_, GLFW_KEY_S) == GLFW_PRESS)
			{
				scene_->GetActiveCamera()->Backward(deltaTime_);
			}

			if (glfwGetKey(appWindow_, GLFW_KEY_D) == GLFW_PRESS)
			{
				scene_->GetActiveCamera()->Right(deltaTime_);
			}

			// glfwPollEvents();
			RenderFrame();
		}
		context_->WaitIdle();
	}

	void CleanupVulkanSwapchain()
	{
		context_->DestroySwapchain();
	}

	void CleanupVulkan()
	{
		// chaletMesh_.reset();
		// CleanupVulkanSwapchain();
		// particleEffect_.reset();
	}
};
