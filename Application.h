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
		camera_ = std::make_unique<Camera>();
		CreateGlfwWindow();
		CreateVulkanInstance();
		CreateVulkanSurface();
		SelectVulkanPhysicalDevice();
		CreateVulkanLogicalDevice();
		CreateVulkanSwapchain();
		CreateVulkanImageViews();
		CreateVulkanRenderPass();
		CreateVulkanDescriptorSetLayout();
		CreateVulkanGraphicsPipeline();
		CreateVulkanCommandPool();
		CreateVulkanDepthBuffer();
		CreateVulkanFramebuffers();

		scene_ = std::make_shared<Scene>(context_->GetDeviceContext(), context_->GetCommandPool());

		chaletMesh_ = scene_->LoadObj("assets/models/chalet/chalet.obj", "assets/models/chalet/chalet.jpg");
		/*particleEffect_ = std::make_unique<ParticleEffect>(logicalDevice_, physicalDevice_, commandPool_, graphicsQueue_, glm::vec3(0.0f, 0.0f, 0.0f), 1, 0.05f);*/
		//CreateVulkanTexture();
		//CreateVulkanVertexBuffer();
		//CreateVulkanIndexBuffer();
		CreateVulkanUniformBuffers();
		CreateVulkanDescriptorPool();

		
		CreateVulkanDescriptorSets();
		CreateVulkanCommandBuffers();
		CreateVulkanSyncObjects();
	}

	~Application()
	{
		DebugMessage("Cleaning up VulkanParticles::Application.");
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
	
	/*
	std::unique_ptr<Texture> rockTexture_;
	std::unique_ptr<Buffer> vertexBuffer_;
	std::unique_ptr<Buffer> indexBuffer_;
	*/

	std::shared_ptr<Scene> scene_;
	
	std::shared_ptr<Mesh> chaletMesh_;
	// std::unique_ptr<ParticleEffect> particleEffect_;

	std::unique_ptr<Camera> camera_;
	
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
	inline static const int MAX_FRAMES_IN_FLIGHT = 2;

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

	void CreateVulkanInstance()
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

	void CreateVulkanImageViews()
	{
		context_->CreateImageViews();
	}

	void CreateVulkanRenderPass()
	{
		context_->CreateRenderPass();
	}

	void CreateVulkanDescriptorSetLayout()
	{
		context_->CreateDescriptorSetLayout();
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

	void CreateVulkanGraphicsPipeline()
	{
		auto vertexShaderSource = ReadFile("assets/shaders/vert.spv");
		auto fragmentShaderSource = ReadFile("assets/shaders/frag.spv");

		context_->CreateGraphicsPipeline(vertexShaderSource, fragmentShaderSource, Vertex::GetVertexInputBindingDescription(), Vertex::GetVertexInputAttributeDescriptions(), VIEWPORT_MIN_DEPTH, VIEWPORT_MAX_DEPTH);
	}

	void CreateVulkanFramebuffers()
	{
		context_->CreateFramebuffers();
	}

	void CreateVulkanCommandPool()
	{
		context_->CreateCommandPool();
	}

	void CreateVulkanDepthBuffer()
	{
		context_->CreateDepthBuffer();
	}

	void CreateVulkanUniformBuffers()
	{
		context_->CreateUniformBuffers(sizeof(ModelViewProj));
	}

	void CreateVulkanDescriptorPool()
	{
		context_->CreateDescriptorPool();
	}

	void CreateVulkanDescriptorSets()
	{
		context_->CreateDescriptorSets(sizeof(ModelViewProj), chaletMesh_->GetTexture()->GenerateDescriptorImageInfo());
	}

	void CreateVulkanCommandBuffers()
	{
		context_->CreateCommandBuffers(chaletMesh_);
	}

	void CreateVulkanSyncObjects()
	{
		context_->CreateSyncObjects(MAX_FRAMES_IN_FLIGHT);
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
		
		app->camera_->UpdateAngle(deltaX, deltaY);	
	}

	void RecreateVulkanSwapchain()
	{
		auto width = 0, height = 0;
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(appWindow_, &width, &height);
			glfwWaitEvents();
		}

		context_->WaitIdle();

		CleanupVulkanSwapchain();

		CreateVulkanSwapchain();
		CreateVulkanImageViews();
		CreateVulkanRenderPass();
		CreateVulkanGraphicsPipeline();
		CreateVulkanDepthBuffer();
		CreateVulkanFramebuffers();
		CreateVulkanUniformBuffers();
		CreateVulkanDescriptorPool();
		CreateVulkanDescriptorSets();
		CreateVulkanCommandBuffers();
	}

	void UpdateVulkanUniformBuffer(const uint32_t index)
	{
		const auto swapchainExtent = context_->GetSwapchainExtent();
		ModelViewProj mvp = {
//			glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			camera_->GetViewMatrix(),
			glm::perspective(glm::radians(45.0f), swapchainExtent.width / static_cast<float>(swapchainExtent.height),
			                 0.1f, 10.0f)
		};

		mvp.Proj[1][1] *= -1.0f;

		context_->UpdateUniformBuffer(index, 0, sizeof(mvp), &mvp);
	}

	void RenderFrame()
	{
		context_->WaitForFences();

		uint32_t imageIndex;
		const auto result = context_->AcquireNextImage(currentFrame, imageIndex);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			RecreateVulkanSwapchain();
			return;
		}

		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Could not acquire Vulkan swapchain image.");
		}

		UpdateVulkanUniformBuffer(imageIndex);

		try
		{
			context_->SubmitFrame(currentFrame, imageIndex);
		}
		catch (VulkanContext::SwapchainOutOfDateException&)
		{
			framebufferResized = false;
			RecreateVulkanSwapchain();
		}
		catch (VulkanContext::PresentSwapchainUnknownException&)
		{
			throw std::runtime_error("Could not present Vulkan swapchain image, VulkanContext::PresentSwapchain failed. Encountered VulkanContext::PresentSwapchainUnknownException.");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(appWindow_))
		{
			// static auto startTime = std::chrono::high_resolution_clock::now();

			// auto currentTime = std::chrono::high_resolution_clock::now();
			// deltaTime_ = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

			static auto lastTime = glfwGetTime();
			auto currentTime = glfwGetTime();
			deltaTime_ = currentTime - lastTime;
			lastTime = currentTime;
			
			if (glfwGetKey(appWindow_, GLFW_KEY_W) == GLFW_PRESS)
			{
				camera_->Forward(deltaTime_);
			}

			if (glfwGetKey(appWindow_, GLFW_KEY_A) == GLFW_PRESS)
			{
				camera_->Left(deltaTime_);
			}

			if (glfwGetKey(appWindow_, GLFW_KEY_S) == GLFW_PRESS)
			{
				camera_->Backward(deltaTime_);
			}

			if (glfwGetKey(appWindow_, GLFW_KEY_D) == GLFW_PRESS)
			{
				camera_->Right(deltaTime_);
			}

			// particleEffect_->Update(deltaTime_);
			
			glfwPollEvents();
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
