#pragma once

#include "GraphicsHeaders.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <fstream>
#include "Vertex.h"
#include "Buffer.h"
#include "Texture.h"

const std::vector<const char*> VK_VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool ENABLE_VK_VALIDATION_LAYERS = false;
static void DebugMessage(const std::string &message) {
}
#else
const bool ENABLE_VK_VALIDATION_LAYERS = true;

static void DebugMessage(const std::string& message)
{
	auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	tm nowTm;
	localtime_s(&nowTm, &now);
	const auto timeString = std::put_time(&nowTm, "%Y-%m-%d %X");
	std::cerr << timeString << " [DEBUG]: " << message << std::endl;
}
#endif

class Application
{
public:
	Application()
	{
		InitializeGlfw();
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
		CreateVulkanFramebuffers();
		CreateVulkanCommandPool();
		CreateVulkanTexture();
		CreateVulkanVertexBuffer();
		CreateVulkanIndexBuffer();
		CreateVulkanUniformBuffers();
		CreateVulkanDescriptorPool();
		CreateVulkanDescriptorSets();
		CreateVulkanCommandBuffers();
		CreateVulkanSyncObjects();
	}

	~Application()
	{
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
	vk::Instance instance_;
	vk::SurfaceKHR surface_;
	std::shared_ptr<vk::PhysicalDevice> physicalDevice_;
	std::shared_ptr<vk::Device> logicalDevice_;
	vk::Queue graphicsQueue_;
	vk::Queue presentQueue_;
	vk::SwapchainKHR swapchain_;
	vk::Format swapchainImageFormat_;
	vk::Extent2D swapchainExtent_;
	std::vector<vk::Image> swapchainImages_;
	std::vector<vk::ImageView> swapchainImageViews_;
	vk::RenderPass renderPass_;
	vk::DescriptorSetLayout descriptorSetLayout_;
	vk::DescriptorPool descriptorPool_;
	std::vector<vk::DescriptorSet> descriptorSets_;
	vk::PipelineLayout graphicsPipelineLayout_;
	vk::Pipeline graphicsPipeline_;
	std::vector<vk::Framebuffer> swapchainFramebuffers_;
	vk::CommandPool commandPool_;
	std::vector<vk::CommandBuffer> commandBuffers_;

	std::unique_ptr<Texture> rockTexture_;

	std::unique_ptr<Buffer> vertexBuffer_;
	std::unique_ptr<Buffer>indexBuffer_;
	std::vector<std::unique_ptr<Buffer>> uniformBuffers_;
	
	std::vector<vk::Semaphore> imageAvailableSemaphores_;
	std::vector<vk::Semaphore> renderFinishedSemaphores_;
	std::vector<vk::Fence> inFlightFences_;
	size_t currentFrame = 0;
	bool framebufferResized = false;

	const int DEFAULT_WINDOW_WIDTH = 640;
	const int DEFAULT_WINDOW_HEIGHT = 480;
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

	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;

		bool IsComplete()
		{
			return GraphicsFamily.has_value() && PresentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR Capabilities;
		std::vector<vk::SurfaceFormatKHR> Formats;
		std::vector<vk::PresentModeKHR> PresentModes;
	};

	struct ModelViewProj {
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
		auto enabledLayers = ENABLE_VK_VALIDATION_LAYERS ? &VK_VALIDATION_LAYERS[0] : nullptr;
		auto requiredGlfwExtensions = getGlfwRequiredExtensions();
		uint32_t requiredExtensionsCount = static_cast<uint32_t>(requiredGlfwExtensions.size());
		const char* const* requiredExtensions = &requiredGlfwExtensions[0];

		const auto applicationInfo = vk::ApplicationInfo(APP_NAME, APP_VERSION, ENGINE_NAME, ENGINE_VERSION,
		                                                 VK_API_VERSION_1_1);
		instance_ = createInstance(vk::InstanceCreateInfo({}, &applicationInfo, enabledLayerCount, enabledLayers,
		                                                  requiredExtensionsCount, requiredExtensions));
	}

	void CreateVulkanSurface()
	{
		VkSurfaceKHR surfaceHandle;
		if (glfwCreateWindowSurface(instance_, appWindow_, nullptr, &surfaceHandle) != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create vk::SurfaceKHR from appWindow.");
		}
		surface_ = surfaceHandle;
	}


	bool checkDeviceExtensionSupport(const vk::PhysicalDevice physicalDevice)
	{
		auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
		std::set<std::string> requiredExtensions(REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices;

		auto queueFamilies = physicalDevice.getQueueFamilyProperties();

		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				indices.GraphicsFamily = i;
			}

			if (queueFamily.queueCount > 0 && physicalDevice.getSurfaceSupportKHR(i, surface_))
			{
				indices.PresentFamily = i;
			}

			if (indices.IsComplete())
			{
				break;
			}

			i++;
		}

		return indices;
	}

	SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice physicalDevice) const
	{
		return {
			physicalDevice.getSurfaceCapabilitiesKHR(surface_),
			physicalDevice.getSurfaceFormatsKHR(surface_),
			physicalDevice.getSurfacePresentModesKHR(surface_),
		};
	}


	bool isVulkanPhysicalDeviceSuitable(const vk::PhysicalDevice physicalDevice)
	{
		auto indices = findQueueFamilies(physicalDevice);
		const auto extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

		auto swapChainAdequate = false;
		if (extensionsSupported)
		{
			const auto swapChainSupport = querySwapChainSupport(physicalDevice);
			swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
		}

		return indices.IsComplete() && extensionsSupported && swapChainAdequate;
	}

	void SelectVulkanPhysicalDevice()
	{
		auto devices = instance_.enumeratePhysicalDevices();

		if (devices.empty())
		{
			throw std::runtime_error(
				"No GPUs were found with Vulkan support! Verify your hardware supports Vulkan 1.1, and your drivers are up-to-date.");
		}

		for (const auto& device : devices)
		{
			if (isVulkanPhysicalDeviceSuitable(device))
			{
				physicalDevice_ = std::make_shared<vk::PhysicalDevice>(device);
				break;
			}
		}

		if (!physicalDevice_)
		{
			throw std::runtime_error(
				"GPUs were found with Vulkan support, but none had queue families with Graphic and Present support. Verify your hardware is supported, and your drivers are up-to-date.");
		}
	}


	void CreateVulkanLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(*physicalDevice_);

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

		auto queuePriority = 1.0f;
		for (auto queueFamily : uniqueQueueFamilies)
		{
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo({}, queueFamily, 1, &queuePriority));
		}

		auto enabledLayerCount = ENABLE_VK_VALIDATION_LAYERS ? static_cast<uint32_t>(VK_VALIDATION_LAYERS.size()) : 0;
		auto enabledLayers = ENABLE_VK_VALIDATION_LAYERS ? &VK_VALIDATION_LAYERS[0] : nullptr;

		vk::PhysicalDeviceFeatures physicalDeviceFeatures;
		logicalDevice_ = std::make_shared<vk::Device>(physicalDevice_->createDevice(vk::DeviceCreateInfo({},
		                                                                   static_cast<uint32_t>(queueCreateInfos.size()
		                                                                   ),
		                                                                   &queueCreateInfos[0], enabledLayerCount,
		                                                                   enabledLayers,
		                                                                   static_cast<uint32_t>(
			                                                                   REQUIRED_DEVICE_EXTENSIONS.size()),
		                                                                   &REQUIRED_DEVICE_EXTENSIONS[0],
		                                                                   &physicalDeviceFeatures)));

		if (!logicalDevice_)
		{
			throw std::runtime_error(
				"Could not create vk::Device. Verify your hardware is supported, and your drivers are up-to-date.");
		}

		graphicsQueue_ = logicalDevice_->getQueue(indices.GraphicsFamily.value(), 0);
		presentQueue_ = logicalDevice_->getQueue(indices.PresentFamily.value(), 0);
	}

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR
				::eSrgbNonlinear)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == vk::PresentModeKHR::eMailbox)
			{
				return availablePresentMode;
			}
		}

		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		int width, height;
		glfwGetFramebufferSize(appWindow_, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width,
		                              std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,
		                               std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}

	void CreateVulkanSwapchain()
	{
		const auto swapChainSupport = querySwapChainSupport(*physicalDevice_);

		const auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.Formats);
		const auto presentMode = chooseSwapPresentMode(swapChainSupport.PresentModes);
		const auto extent = chooseSwapExtent(swapChainSupport.Capabilities);
		auto indices = findQueueFamilies(*physicalDevice_);

		auto imageCount = swapChainSupport.Capabilities.minImageCount + 1;
		if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.Capabilities.maxImageCount;
		}

		const auto imageSharingMode = (indices.GraphicsFamily != indices.PresentFamily)
			                              ? vk::SharingMode::eConcurrent
			                              : vk::SharingMode::eExclusive;
		const auto queueFamilyIndexCount = (indices.GraphicsFamily != indices.PresentFamily) ? 2 : 0;
		auto queueFamilyIndices = (indices.GraphicsFamily != indices.PresentFamily)
			                          ? std::vector<uint32_t>({
				                          indices.GraphicsFamily.value(), indices.PresentFamily.value()
			                          })
			                          : std::vector<uint32_t>();
		swapchain_ = logicalDevice_->createSwapchainKHR(vk::SwapchainCreateInfoKHR({}, surface_,
		                                                                          imageCount,
		                                                                          surfaceFormat.format,
		                                                                          surfaceFormat.colorSpace,
		                                                                          extent, 1,
		                                                                          vk::ImageUsageFlagBits::
		                                                                          eColorAttachment,
		                                                                          imageSharingMode,
		                                                                          queueFamilyIndexCount,
		                                                                          !queueFamilyIndices.empty()
			                                                                          ? &queueFamilyIndices[0]
			                                                                          : nullptr,
		                                                                          swapChainSupport
		                                                                          .Capabilities.currentTransform,
		                                                                          vk::CompositeAlphaFlagBitsKHR::
		                                                                          eOpaque,
		                                                                          presentMode, VK_TRUE));

		if (!swapchain_)
		{
			throw std::runtime_error(
				"Could not create vk::SwapchainKHR. Verify your hardware is supported, and your drivers are up-to-date.");
		}

		swapchainImages_ = logicalDevice_->getSwapchainImagesKHR(swapchain_);
		swapchainImageFormat_ = surfaceFormat.format;
		swapchainExtent_ = extent;
	}

	void CreateVulkanImageViews()
	{
		for (const auto image : swapchainImages_)
		{
			auto imageView = logicalDevice_->createImageView(vk::ImageViewCreateInfo(
				{}, image,
				vk::ImageViewType::e2D, swapchainImageFormat_,
				{},
				vk::ImageSubresourceRange(
					vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
			if (!imageView)
			{
				throw std::runtime_error("Could not create vk::ImageView for given swapChainImage.");
			}
			swapchainImageViews_.push_back(imageView);
		}
	}

	void CreateVulkanRenderPass()
	{
		vk::AttachmentDescription colorAttachment({}, swapchainImageFormat_,
		                                          vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
		                                          vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
		                                          vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
		                                          vk::ImageLayout::ePresentSrcKHR);
		vk::AttachmentReference colorAttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
		vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1,
		                               &colorAttachmentReference);
		vk::SubpassDependency subpassDependency(
			VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, {},
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
		renderPass_ = logicalDevice_->createRenderPass(vk::RenderPassCreateInfo({}, 1, &colorAttachment, 1,
		                                                                       &subpass, 1, &subpassDependency));

		if (!renderPass_)
		{
			throw std::runtime_error(
				"Could not create vk::RenderPass. Verify your hardware is supported, and your drivers are up-to-date.");
		}
	}

	void CreateVulkanDescriptorSetLayout()
	{
		vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, 1, &descriptorSetLayoutBinding);
		descriptorSetLayout_ = logicalDevice_->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
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

	vk::ShaderModule CreateShaderModule(const std::stringstream& code) const
	{
		const auto codeString = code.str();
		return logicalDevice_->createShaderModule(vk::ShaderModuleCreateInfo(
			{}, codeString.length(), reinterpret_cast<const uint32_t*>(&codeString[0])));
	}

	void CreateVulkanGraphicsPipeline()
	{
		auto vertexShaderCode = ReadFile("assets/shaders/vert.spv");
		auto fragmentShaderCode = ReadFile("assets/shaders/frag.spv");

		auto vertexShaderModule = CreateShaderModule(vertexShaderCode);
		auto fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

		vk::PipelineShaderStageCreateInfo vertexShaderPipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eVertex, vertexShaderModule, "main");

		vk::PipelineShaderStageCreateInfo fragmentShaderPipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eFragment, fragmentShaderModule, "main");

		vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[] = {
			vertexShaderPipelineShaderStageCreateInfo, fragmentShaderPipelineShaderStageCreateInfo
		};
		auto bindingDescription = Vertex::GetVertexInputBindingDescription();
		auto attributeDescriptions = Vertex::GetVertexInputAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo({}, 1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), &attributeDescriptions[0]);
		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
			{}, vk::PrimitiveTopology::eTriangleList);
		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapchainExtent_.width),
		                      static_cast<float>(swapchainExtent_.height), VIEWPORT_MIN_DEPTH, VIEWPORT_MAX_DEPTH);
		vk::Rect2D scissor({0, 0}, swapchainExtent_);
		vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo({}, 1, &viewport, 1, &scissor);
		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
			{}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise,
			VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f);
		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
			{}, vk::SampleCountFlagBits::e1, VK_FALSE);
		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(VK_FALSE, {}, {}, {}, {}, {}, {},
		                                                                        vk::ColorComponentFlagBits::eR | vk::
		                                                                        ColorComponentFlagBits::eG | vk::
		                                                                        ColorComponentFlagBits::eB | vk::
		                                                                        ColorComponentFlagBits::eA);
		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
			{}, VK_FALSE, vk::LogicOp::eCopy, 1, &pipelineColorBlendAttachmentState);

		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, 1, &descriptorSetLayout_);
		graphicsPipelineLayout_ = logicalDevice_->createPipelineLayout(pipelineLayoutCreateInfo);
		if (!graphicsPipelineLayout_)
		{
			throw std::runtime_error(
				"Could not create vk::PipelineLayout for the Vulkan graphics pipeline. Verify your hardware is supported, and your drivers are up-to-date.");
		}

		graphicsPipeline_ = logicalDevice_->createGraphicsPipeline(nullptr, {
			                                      {}, 2, pipelineShaderStageCreateInfos,
			                                      &pipelineVertexInputStateCreateInfo,
			                                      &pipelineInputAssemblyStateCreateInfo, nullptr,
			                                      &pipelineViewportStateCreateInfo,
			                                      &pipelineRasterizationStateCreateInfo,
			                                      &pipelineMultisampleStateCreateInfo, nullptr,
			                                      &pipelineColorBlendStateCreateInfo, nullptr,
			                                      graphicsPipelineLayout_, renderPass_
		                                      });
		if (!graphicsPipeline_)
		{
			throw std::runtime_error(
				"Could not create vk::Pipeline for the Vulkan graphics pipeline. Verify your hardware is supported, and your drivers are up-to-date.");
		}

		logicalDevice_->destroyShaderModule(vertexShaderModule);
		logicalDevice_->destroyShaderModule(fragmentShaderModule);
	}

	void CreateVulkanFramebuffers()
	{
		for (const auto imageView : swapchainImageViews_)
		{
			const vk::ImageView attachments[] = {imageView};
			vk::FramebufferCreateInfo framebufferCreateInfo({}, renderPass_, 1, attachments, swapchainExtent_.width,
			                                                swapchainExtent_.height, 1);
			auto framebuffer = logicalDevice_->createFramebuffer(framebufferCreateInfo);
			if (!framebuffer)
			{
				throw std::runtime_error("Could not create vk::Framebuffer for given imageView.");
			}
			swapchainFramebuffers_.push_back(framebuffer);
		}
	}

	void CreateVulkanCommandPool()
	{
		auto queueFamilyIndices = findQueueFamilies(*physicalDevice_);
		const vk::CommandPoolCreateInfo commandPoolCreateInfo({}, queueFamilyIndices.GraphicsFamily.value());
		commandPool_ = logicalDevice_->createCommandPool(commandPoolCreateInfo);
		if (!commandPool_)
		{
			throw std::runtime_error(
				"Could not create vk::CommandPool. Verify your hardware is supported, and your drivers are up-to-date.");
		}
	}

	void CreateVulkanTexture()
	{
		rockTexture_.reset(new Texture("assets/textures/rocks_05/col.jpg", logicalDevice_, physicalDevice_, commandPool_, graphicsQueue_));
	}

	void CreateVulkanVertexBuffer()
	{
		const auto bufferSize = sizeof(vertices[0]) * vertices.size();

		const auto stagingBuffer = std::make_unique<Buffer>(logicalDevice_, physicalDevice_, bufferSize,
		                                                    vk::BufferUsageFlagBits::eTransferSrc,
		                                                    vk::SharingMode::eExclusive,
		                                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::
		                                                    MemoryPropertyFlagBits::eHostCoherent);
		stagingBuffer->Fill(0, bufferSize, &vertices[0]);
		vertexBuffer_.reset(new Buffer(logicalDevice_, physicalDevice_, bufferSize,
		                               vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		                               {}, vk::MemoryPropertyFlagBits::eDeviceLocal));
		Buffer::Copy(stagingBuffer, vertexBuffer_, bufferSize, commandPool_, graphicsQueue_);
	}

	void CreateVulkanIndexBuffer()
	{
		const auto bufferSize = sizeof(indices[0]) * indices.size();
		const auto stagingBuffer = std::make_unique<Buffer>(logicalDevice_, physicalDevice_, bufferSize,
		                                                    vk::BufferUsageFlagBits::eTransferSrc,
		                                                    vk::SharingMode::eExclusive,
		                                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::
		                                                    MemoryPropertyFlagBits::eHostCoherent);
		stagingBuffer->Fill(0, bufferSize, &indices[0]);
		indexBuffer_.reset(new Buffer(logicalDevice_, physicalDevice_, bufferSize,
		                              vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, {},
		                              vk::MemoryPropertyFlagBits::eDeviceLocal));
		Buffer::Copy(stagingBuffer, indexBuffer_, bufferSize, commandPool_, graphicsQueue_);
	}

	void CreateVulkanUniformBuffers()
	{
		auto bufferSize = sizeof(ModelViewProj);
		uniformBuffers_.resize(swapchainImages_.size());

		for (size_t i = 0; i < swapchainImages_.size(); ++i)
		{
			uniformBuffers_[i].reset(new Buffer(logicalDevice_, physicalDevice_, bufferSize,
			                                    vk::BufferUsageFlagBits::eUniformBuffer, {},
			                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::
			                                    eHostCoherent));
		}
	}

	void CreateVulkanDescriptorPool()
	{
		vk::DescriptorPoolSize descriptorPoolSize(vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(swapchainImages_.size()));
		descriptorPool_ = logicalDevice_->createDescriptorPool({ {}, static_cast<uint32_t>(swapchainImages_.size()), 1,& descriptorPoolSize });
	}

	void CreateVulkanDescriptorSets()
	{
		std::vector<vk::DescriptorSetLayout> descriptorSetLayoutTemplates(swapchainImages_.size(), descriptorSetLayout_);
		descriptorSets_ = logicalDevice_->allocateDescriptorSets({
			descriptorPool_, static_cast<uint32_t>(swapchainImages_.size()), &descriptorSetLayoutTemplates[0]
		});
		for (size_t i = 0; i < swapchainImages_.size(); ++i)
		{
			vk::DescriptorBufferInfo descriptorBufferInfo(uniformBuffers_[i]->GetHandle(), 0, sizeof(ModelViewProj));
			logicalDevice_->updateDescriptorSets(
				{
					{
						descriptorSets_[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, {},
						&descriptorBufferInfo
					}
				}, nullptr);
		}
	}

	void CreateVulkanCommandBuffers()
	{
		const vk::CommandBufferAllocateInfo commandBufferAllocateInfo(commandPool_, vk::CommandBufferLevel::ePrimary,
		                                                              static_cast<uint32_t>(swapchainFramebuffers_.
			                                                              size()));
		commandBuffers_ = logicalDevice_->allocateCommandBuffers(commandBufferAllocateInfo);
		if (commandBuffers_.empty())
		{
			throw std::runtime_error(
				"Could not allocate vk::CommandBuffer(s). Verify your hardware is supported, and your drivers are up-to-date.");
		}

		for (auto i = 0; i < commandBuffers_.size(); i++)
		{
			const auto& buffer = commandBuffers_[i];
			vk::CommandBufferBeginInfo commandBufferBeginInfo;
			buffer.begin(commandBufferBeginInfo);
			//throw std::runtime_error("Could not begin recording vk::CommandBuffer. Verify your hardware is supported, and your drivers are up-to-date.");
			const vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});
			vk::RenderPassBeginInfo renderPassBeginInfo(renderPass_, swapchainFramebuffers_[i],
			                                            {{0, 0}, swapchainExtent_}, 1, &clearColor);

			buffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
			buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline_);
			buffer.bindVertexBuffers(0, vertexBuffer_->GetHandle(), static_cast<vk::DeviceSize>(0));
			buffer.bindIndexBuffer(indexBuffer_->GetHandle(), 0, vk::IndexType::eUint16);
			buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipelineLayout_, 0, 1,
			                          &descriptorSets_[i], 0, nullptr);
			buffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
			buffer.endRenderPass();
			buffer.end();
		}
	}

	void CreateVulkanSyncObjects()
	{
		const vk::SemaphoreCreateInfo semaphoreCreateInfo;
		const vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto imageAvailableSemaphore = logicalDevice_->createSemaphore(semaphoreCreateInfo);
			if (!imageAvailableSemaphore)
			{
				throw std::runtime_error(
					"Could not create vk::Semaphore(s) for frame [" + std::to_string(i) +
					"] - imageAvailableSemaphore.");
			}
			imageAvailableSemaphores_.push_back(imageAvailableSemaphore);
			auto renderFinishedSemaphore = logicalDevice_->createSemaphore(semaphoreCreateInfo);
			if (!renderFinishedSemaphore)
			{
				throw std::runtime_error(
					"Could not create vk::Semaphore(s) for frame [" + std::to_string(i) +
					"] - renderFinishedSemaphore.");
			}
			renderFinishedSemaphores_.push_back(renderFinishedSemaphore);
			auto inFlightFence = logicalDevice_->createFence(fenceCreateInfo);
			if (!inFlightFence)
			{
				throw std::runtime_error(
					"Could not create vk::Fence(s) for frame [" + std::to_string(i) + "] - inFlightFence.");
			}
			inFlightFences_.push_back(inFlightFence);
		}
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

	void RecreateVulkanSwapchain()
	{
		auto width = 0, height = 0;
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(appWindow_, &width, &height);
			glfwWaitEvents();
		}

		logicalDevice_->waitIdle();

		CleanupVulkanSwapchain();

		CreateVulkanSwapchain();
		CreateVulkanImageViews();
		CreateVulkanRenderPass();
		CreateVulkanGraphicsPipeline();
		CreateVulkanFramebuffers();
		CreateVulkanUniformBuffers();
		CreateVulkanDescriptorPool();
		CreateVulkanDescriptorSets();
		CreateVulkanCommandBuffers();
	}

	void UpdateVulkanUniformBuffer(const uint32_t index)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		auto deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		ModelViewProj mvp = {
			glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::perspective(glm::radians(45.0f), swapchainExtent_.width / static_cast<float>(swapchainExtent_.height),
			                 0.1f, 10.0f)
		};

		mvp.Proj[1][1] *= -1.0f;

		uniformBuffers_[index]->Fill(0, sizeof(mvp), &mvp);
	}

	void RenderFrame()
	{
		auto result = logicalDevice_->waitForFences(inFlightFences_, VK_TRUE, UINT64_MAX);

		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Could not wait for Vulkan fences.");
		}
		
		uint32_t imageIndex;
		result = logicalDevice_->acquireNextImageKHR(swapchain_, UINT64_MAX,
		                                                 imageAvailableSemaphores_[currentFrame], nullptr, &imageIndex);

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
		
		vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame]};
		vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
		vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame]};
		const vk::SubmitInfo submitInfo(1, waitSemaphores, waitStages, 1, &commandBuffers_[imageIndex], 1, signalSemaphores);

		logicalDevice_->resetFences(inFlightFences_[currentFrame]);
		graphicsQueue_.submit(submitInfo, inFlightFences_[currentFrame]);

		vk::SwapchainKHR swapchains[] = {swapchain_};
		const vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, swapchains, &imageIndex);
		try {
			presentQueue_.presentKHR(presentInfo);
		} catch (vk::OutOfDateKHRError e)
		{
			framebufferResized = false;
			RecreateVulkanSwapchain();
		} catch (...)
		{
			throw std::runtime_error("Could not present Vulkan swapchain image.");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(appWindow_))
		{
			glfwPollEvents();
			RenderFrame();
		}
		logicalDevice_->waitIdle();
	}

	void CleanupVulkanSwapchain()
	{
		for (const auto framebuffer : swapchainFramebuffers_)
		{
			logicalDevice_->destroyFramebuffer(framebuffer);
		}
		swapchainFramebuffers_.clear();

		logicalDevice_->freeCommandBuffers(commandPool_, commandBuffers_);
		commandBuffers_.clear();
		
		logicalDevice_->destroyPipeline(graphicsPipeline_);
		logicalDevice_->destroyPipelineLayout(graphicsPipelineLayout_);
		logicalDevice_->destroyRenderPass(renderPass_);

		for (size_t i = 0; i < uniformBuffers_.size(); i++) {
			uniformBuffers_[i].reset();
		}
		
		logicalDevice_->destroyDescriptorPool(descriptorPool_);
		
		for (const auto imageView : swapchainImageViews_)
		{
			logicalDevice_->destroyImageView(imageView);
		}
		swapchainImageViews_.clear();
		
		logicalDevice_->destroySwapchainKHR(swapchain_);
	}

	void CleanupVulkan()
	{
		CleanupVulkanSwapchain();
		logicalDevice_->destroyDescriptorSetLayout(descriptorSetLayout_);
		indexBuffer_.reset();
		vertexBuffer_.reset();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			logicalDevice_->destroySemaphore(renderFinishedSemaphores_[i]);
			logicalDevice_->destroySemaphore(imageAvailableSemaphores_[i]);
			logicalDevice_->destroyFence(inFlightFences_[i]);
		}

		logicalDevice_->destroyCommandPool(commandPool_);
		logicalDevice_->destroy();

		instance_.destroySurfaceKHR(surface_);
		instance_.destroy();
	}
};
