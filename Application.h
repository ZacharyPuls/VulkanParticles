#pragma once

#include "GraphicsHeaders.h"
#include "Vertex.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "ShaderModule.h"
#include "GraphicsPipeline.h"
#include "CommandPool.h"
#include "PhysicalDevice.h"
#include "ImageView.h"
#include "SwapChain.h"
#include "VertexBuffer.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <optional>
#include <array>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <set>
#include <sstream>

const std::vector<const char*> VK_VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool ENABLE_VK_VALIDATION_LAYERS = false;
static void DebugMessage(const std::string &message) {
}
#else
const bool ENABLE_VK_VALIDATION_LAYERS = true;
static void DebugMessage(const std::string& message) {/*
	auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	auto timeString = std::put_time(std::localtime_s(&now), "%Y-%m-%d %X");
	std::cerr << timeString << " [DEBUG]: " << message << std::endl;
*/}
#endif

class Application
{
public:
	Application() {
		initializeGlfw();
		createGlfwWindow();
		initializeVulkan();
	}

	~Application() {
		destroyVulkanSwapChain();
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}
		delete commandPool;
		delete triangleVertexBuffer;
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(appWindow);
		glfwTerminate();
	}

	void Run() {
		mainLoop();
	}

private:
	VkInstance instance;
	VkSurfaceKHR surface;
	PhysicalDevice *physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	SwapChain* swapChain;
	std::vector<ImageView*> swapChainImageViews;
	RenderPass* renderPass;
	GraphicsPipeline* graphicsPipeline;
	std::vector<Framebuffer*> swapChainFramebuffers;
	CommandPool* commandPool;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	VertexBuffer* triangleVertexBuffer;
	size_t currentFrame = 0;
	GLFWwindow* appWindow;

	const uint32_t DEFAULT_WINDOW_WIDTH = 640;
	const uint32_t DEFAULT_WINDOW_HEIGHT = 480;
	const size_t MAX_FRAMES_IN_FLIGHT = 2;
	const std::vector<const char *> REQUIRED_VULKAN_DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentationFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentationFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	void initializeGlfw() {
		if (!glfwInit()) {
			throw std::runtime_error("Could not initialize GLFW.");
		}

		if (!glfwVulkanSupported()) {
			throw std::runtime_error("Vulkan is not supported on this platform.");
		}

		glfwSetErrorCallback(throwGlfwErrorAsException);
	}

	void createGlfwWindow() {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		appWindow = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, "Vulkan Particles!", nullptr, nullptr);

		glfwSetKeyCallback(appWindow, keyboardCallback);

		if (!appWindow) {
			throw std::runtime_error("Could not create app window.");
		}
	}

	void initializeVulkan() {
		createVulkanInstance();
		createVulkanSurface();
		selectVulkanDevice();
		createVulkanLogicalDevice();
		createVulkanSwapChain();
		createVulkanImageViews();
		renderPass = new RenderPass(device, swapChain->GetImageFormat());
		createVulkanGraphicsPipeline();
		createVulkanFramebuffers();
		createVulkanCommandPool();
		createVulkanCommandBuffers();
		createVulkanSyncObjects();
	}

	void createVulkanInstance() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		VkApplicationInfo applicationInfo = {};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pApplicationName = "Vulkan Particles!";
		applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		applicationInfo.pEngineName = "No Engine";
		applicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
		applicationInfo.apiVersion = VK_API_VERSION_1_1;
		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledExtensionCount = glfwExtensionCount;
		instanceCreateInfo.ppEnabledExtensionNames = glfwExtensions;
		instanceCreateInfo.enabledLayerCount = 0;

		if (ENABLE_VK_VALIDATION_LAYERS && isVkValidationLayerSupported()) {
			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VK_VALIDATION_LAYERS.size());
			instanceCreateInfo.ppEnabledLayerNames = &VK_VALIDATION_LAYERS[0];
		}

		if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("Could not create Vulkan instance.");
		}
	}

	void createVulkanSurface() {
		if (glfwCreateWindowSurface(instance, appWindow, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create surface for GLFW window and previously created Vulkan instance.");
		}
	}
	
	void selectVulkanDevice() {
		const auto availableDevices = PhysicalDevice::Enumerate(instance, surface);

		for (auto availableDevice : availableDevices) {
			if (isVulkanDeviceSuitable(availableDevice, surface)) {
				physicalDevice = availableDevice;
				return;
			}
		}

		throw std::runtime_error("Found " + std::to_string(availableDevices.size()) + " available GPU(s) with Vulkan support, but none were suitable.");
	}

	bool isVulkanDeviceSuitable(PhysicalDevice*& physicalDevice, const VkSurfaceKHR& surface)
	{
		auto requiredExtensionsSupported = physicalDevice->SupportsExtensions(REQUIRED_VULKAN_DEVICE_EXTENSIONS);
		auto swapChainSuitable = false;
		if (requiredExtensionsSupported)
		{
			const auto swapChainSupport = SwapChain::SupportDetails::Query(physicalDevice->GetHandle(), surface);
			swapChainSuitable = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}
		return physicalDevice->GetQueueFamilyIndices(surface).IsComplete() && requiredExtensionsSupported && swapChainSuitable;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		std::vector<VkSurfaceFormatKHR>::const_iterator format = std::find_if(availableFormats.begin(), availableFormats.end(), [](auto availableFormat) -> bool { return availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; });
		if (format != availableFormats.end()) {
			return *format;
		}
		else {
			return availableFormats[0];
		}
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		if (std::find(availablePresentModes.begin(), availablePresentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != availablePresentModes.end()) {
			return VK_PRESENT_MODE_MAILBOX_KHR;
		}
		else {
			return VK_PRESENT_MODE_FIFO_KHR;
		}
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else {
			int width;
			int height;

			glfwGetFramebufferSize(appWindow, &width, &height);

			return { std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
				std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
		}
	}

	void createVulkanLogicalDevice() {
		auto queueFamilyIndices = physicalDevice->GetQueueFamilyIndices(surface);

		float queuePriorities[] = { 1.0f };

		VkDeviceQueueCreateInfo deviceGraphicsQueueCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value(),
			.queueCount = 1,
			.pQueuePriorities = queuePriorities
		};
		
		VkDeviceQueueCreateInfo devicePresentationQueueCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queueFamilyIndices.PresentFamily.value(),
			.queueCount = 1,
			.pQueuePriorities = queuePriorities
		};
		
		VkDeviceQueueCreateInfo deviceQueueCreateInfos[] = { deviceGraphicsQueueCreateInfo, devicePresentationQueueCreateInfo };

		VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
		
		VkDeviceCreateInfo deviceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = deviceQueueCreateInfos,
			.enabledExtensionCount = static_cast<uint32_t>(REQUIRED_VULKAN_DEVICE_EXTENSIONS.size()),
			.ppEnabledExtensionNames = &REQUIRED_VULKAN_DEVICE_EXTENSIONS[0],
			.pEnabledFeatures = &physicalDeviceFeatures
		};
		
		if (ENABLE_VK_VALIDATION_LAYERS) {
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VK_VALIDATION_LAYERS.size());
			deviceCreateInfo.ppEnabledLayerNames = &VK_VALIDATION_LAYERS[0];
		}
		else {
			deviceCreateInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice->GetHandle(), &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical Vulkan device for selected GPU.");
		}

		vkGetDeviceQueue(device, queueFamilyIndices.GraphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, queueFamilyIndices.PresentFamily.value(), 0, &presentationQueue);
	}

	void createVulkanSwapChain() {
		swapChain = new SwapChain(device, physicalDevice, surface, appWindow);
	}

	void createVulkanImageViews() {
		swapChainImageViews.resize(swapChain->GetImages().size());
		auto i = 0;
		for (auto& swapChainImage : swapChain->GetImages()) {
			swapChainImageViews[i++] = new ImageView(device, swapChainImage, swapChain->GetImageFormat());
		}
	}

	static std::stringstream readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file with filename [" + filename + "].");
		}
		
		std::stringstream buffer;
		buffer << file.rdbuf();
	
		file.close();

		return buffer;
	}

	void createVulkanGraphicsPipeline()
	{
		const auto vertexShaderSource = readFile("assets/shaders/vert.spv");
		const auto fragmentShaderSource = readFile("assets/shaders/frag.spv");
		graphicsPipeline = new GraphicsPipeline(device, vertexShaderSource, fragmentShaderSource, "main", swapChain->GetExtent(), renderPass);
	}

	void createVulkanFramebuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size());
		
		for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
			const VkImageView attachments[] = {
				swapChainImageViews[i]->GetHandle()
			};

			swapChainFramebuffers[i] = new Framebuffer(device, renderPass, attachments, swapChain->GetExtent());
		}
	}

	void createVulkanCommandPool() {
		const auto queueFamilyIndices = physicalDevice->GetQueueFamilyIndices(surface);
		commandPool = new CommandPool(device, queueFamilyIndices.GraphicsFamily.value());
	}

	void createVulkanCommandBuffers() {
		const std::vector<Vertex> triangleVertices = { {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}} };
		triangleVertexBuffer = new VertexBuffer(device, physicalDevice, triangleVertices);
		commandPool->SubmitRenderPass(renderPass, swapChainFramebuffers, swapChain->GetExtent(), graphicsPipeline, triangleVertexBuffer);
	}

	void createVulkanSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};

		VkFenceCreateInfo fenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};
		
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create Vulkan sync objects for frame with index [" + std::to_string(i) + "].");
			}
		}
	}

	static void throwGlfwErrorAsException(int error, const char* description) {
		const auto exceptionMessage = "GLFW encountered an error with code [" + std::to_string(error) + "], and description [" + std::string(description) + "].";
		throw std::runtime_error(exceptionMessage.c_str());
	}

	static void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}

	bool isVkValidationLayerSupported() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, &availableLayers[0]);

		for (const char* layerName : VK_VALIDATION_LAYERS) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	void renderFrame() {
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		auto result = vkAcquireNextImageKHR(device, swapChain->GetHandle(), UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateVulkanSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
		}

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		
		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = waitSemaphores,
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = commandPool->GetBuffer(imageIndex),
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphores
		};

		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit Vulkan draw command buffer.");
		}
		VkSwapchainKHR swapChains[] = { swapChain->GetHandle() };
		
		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signalSemaphores,
			.swapchainCount = 1,
			.pSwapchains = swapChains,
			.pImageIndices = &imageIndex,
			.pResults = nullptr
		};

		result = vkQueuePresentKHR(presentationQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			recreateVulkanSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present Vulkan swap chain image.");
		}

		currentFrame = ++currentFrame % MAX_FRAMES_IN_FLIGHT;
	}

	void destroyVulkanSwapChain() {
		for (auto framebuffer : swapChainFramebuffers) {
			delete framebuffer;
		}
		commandPool->FreeBuffers();
		delete graphicsPipeline;
		delete renderPass;
		for (auto imageView : swapChainImageViews) {
			delete imageView;
		}
		delete swapChain;
	}

	void recreateVulkanSwapChain() {
		vkDeviceWaitIdle(device);

		destroyVulkanSwapChain();

		createVulkanSwapChain();
		createVulkanImageViews();
		renderPass = new RenderPass(device, swapChain->GetImageFormat());
		createVulkanGraphicsPipeline();
		createVulkanFramebuffers();
		createVulkanCommandBuffers();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(appWindow)) {
			glfwPollEvents();
			renderFrame();
		}

		vkDeviceWaitIdle(device);
	}
};

