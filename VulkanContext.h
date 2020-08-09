#pragma once

#include "stdafx.h"
#include <set>
#include <optional>


#include "DescriptorSet.h"
#include "Image.h"
#include "VulkanDeviceContext.h"
#include "Mesh.h"
#include "VulkanParticlesException.h"

class VulkanContext
{
public:

	VulkanContext(std::vector<const char*> enabledLayers, std::vector<const char*> extensions,
		const std::string& appName, const uint32_t appVersion, const std::string& engineName,
		const uint32_t engineVersion) : enabledLayers_(enabledLayers)
	{
		const uint32_t enabledLayerCount = enabledLayers_.size();
		uint32_t enabledExtensionsCount = extensions.size();
		auto enabledExtensions = &extensions[0];

		const auto applicationInfo = vk::ApplicationInfo(appName.c_str(), appVersion, engineName.c_str(), engineVersion,
			VK_API_VERSION_1_2);

		auto enables = vk::ValidationFeatureEnableEXT{ VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
		auto features = vk::ValidationFeaturesEXT{ 1, &enables };

		const char* const* enabledLayersPtr = (enabledLayerCount > 0) ? &enabledLayers_[0] : nullptr;
		
		auto instanceCreateInfo = vk::InstanceCreateInfo({}, &applicationInfo, enabledLayerCount, enabledLayersPtr,
		                                   enabledExtensionsCount, enabledExtensions);

		instanceCreateInfo.pNext = &features;
		instance_ = vk::createInstanceUnique(instanceCreateInfo);
	}
	
	~VulkanContext()
	{
		// TODO: [zpuls 2020-07-31T18:14] Add better error handling to VulkanContext::~VulkanContext()
		DebugMessage("Cleaning up VulkanParticles::VulkanContext.");
	}

	vk::Extent2D GetSwapchainExtent()
	{
		return swapchainExtent_;
	}

	VulkanDeviceContext GetDeviceContext()
	{
		return deviceContext_;
	}

	vk::UniqueCommandPool& GetCommandPool()
	{
		return commandPool_;
	}

	class SwapchainOutOfDateException : public VulkanParticlesException
	{
	public:
		SwapchainOutOfDateException()
			: VulkanParticlesException(ExceptionType::SWAPCHAIN_OUT_OF_DATE,
			                           "Could not present Vulkan swapchain image, VulkanContext::PresentSwapchain failed.")
		{
		}
	};

	class PresentSwapchainUnknownException : public VulkanParticlesException
	{
	public:
		PresentSwapchainUnknownException() : VulkanParticlesException(ExceptionType::PRESENT_SWAPCHAIN_UNKNOWN_ERROR,
		                                                              "Could not present Vulkan swapchain image, unknown exception.")
		{
			
		}
	};

	// might want to hard-code descriptorsetlayouts in here for now, to enforce separation of concerns.
	void Initialize(GLFWwindow* appWindow, const vk::Extent2D swapchainExtent, const vk::Format imageFormat, const std::stringstream& vertexShaderSource, const std::stringstream& fragmentShaderSource, const float minDepth, const float maxDepth, const int maxFramesInFlight)
	{
		CreateSurface(appWindow);
		SelectPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapchain(swapchainExtent);
		CreateImageViews(imageFormat);
		CreateRenderPass(imageFormat);
		CreateDescriptorPool();
		CreateCommandPool();

		// TODO: [zpuls 2020-08-08T15:55] Figure out wtf to do about dynamic descriptorset/layout binding....NVIDIA only supports 8 bound at a time? Not sure I understand the concept fully.
		vk::DescriptorSetLayoutBinding uboLayoutBinding = { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
		vk::DescriptorSetLayoutBinding samplerLayoutBinding = { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment };
		
		const auto descriptorSetLayoutIndex = AddDescriptorSetLayout({ uboLayoutBinding, samplerLayoutBinding });
		
		CreateGraphicsPipeline(vertexShaderSource, fragmentShaderSource, Vertex::GetVertexInputBindingDescription(), Vertex::GetVertexInputAttributeDescriptions(), minDepth, maxDepth);
		CreateDepthBuffer();
		CreateFramebuffers();
		// create descriptor sets here?
		CreateCommandBuffers();
		CreateSyncObjects(maxFramesInFlight);
	}
	
	void CreateSurface(GLFWwindow* appWindow)
	{
		VkSurfaceKHR surfaceHandle;
		if (glfwCreateWindowSurface(*instance_, appWindow, nullptr, &surfaceHandle) != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create vk::SurfaceKHR from appWindow.");
		}
		vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> _deleter(instance_.get());
		surface_ = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surfaceHandle), _deleter);
	}

	void SelectPhysicalDevice()
	{
		auto devices = instance_->enumeratePhysicalDevices();

		if (devices.empty())
		{
			throw std::runtime_error(
				"No GPUs were found with Vulkan support! Verify your hardware supports Vulkan 1.1, and your drivers are up-to-date.");
		}

		for (const auto& device : devices)
		{
			if (isVulkanPhysicalDeviceSuitable_(device))
			{
				deviceContext_.PhysicalDevice = std::make_shared<vk::PhysicalDevice>(device);
				break;
			}
		}

		if (!deviceContext_.PhysicalDevice)
		{
			throw std::runtime_error(
				"GPUs were found with Vulkan support, but none had queue families with Graphic and Present support. Verify your hardware is supported, and your drivers are up-to-date.");
		}
	}

	void CreateLogicalDevice()
	{
		auto indices = findQueueFamilies_(*deviceContext_.PhysicalDevice);

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };

		auto queuePriority = 1.0f;
		for (auto queueFamily : uniqueQueueFamilies)
		{
			queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo({}, queueFamily, 1, &queuePriority));
		}

		auto enabledLayerCount = enabledLayers_.size();
		auto enabledLayers = enabledLayerCount > 0 ? &enabledLayers_[0] : nullptr;

		vk::PhysicalDeviceFeatures physicalDeviceFeatures;
		physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceContext_.LogicalDevice = std::make_shared<vk::Device>(deviceContext_.PhysicalDevice->createDevice(vk::DeviceCreateInfo({},
			static_cast<
			uint32_t>(
				queueCreateInfos
				.size()
				),
			&queueCreateInfos
			[0],
			enabledLayerCount,
			enabledLayers,
			static_cast<
			uint32_t>(
				REQUIRED_DEVICE_EXTENSIONS
				.size()),
			&REQUIRED_DEVICE_EXTENSIONS
			[0],
			&physicalDeviceFeatures)));

		if (!deviceContext_.LogicalDevice)
		{
			throw std::runtime_error(
				"Could not create vk::Device. Verify your hardware is supported, and your drivers are up-to-date.");
		}

		deviceContext_.GraphicsQueue = std::make_shared<vk::Queue>(deviceContext_.LogicalDevice->getQueue(indices.GraphicsFamily.value(), 0));
		deviceContext_.PresentQueue = std::make_shared<vk::Queue>(deviceContext_.LogicalDevice->getQueue(indices.PresentFamily.value(), 0));
	}

	void CreateSwapchain(const vk::Extent2D& requestedSwapchainExtent)
	{
		const auto swapChainSupport = querySwapChainSupport_(*deviceContext_.PhysicalDevice);

		const auto surfaceFormat = chooseSwapSurfaceFormat_(swapChainSupport.Formats);
		const auto presentMode = chooseSwapPresentMode_(swapChainSupport.PresentModes);
		const auto extent = chooseSwapExtent_(swapChainSupport.Capabilities, requestedSwapchainExtent);
		auto indices = findQueueFamilies_(*deviceContext_.PhysicalDevice);

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

		swapchain_ = deviceContext_.LogicalDevice->createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR(
			{}, *surface_, imageCount, surfaceFormat.format, surfaceFormat.colorSpace,
			extent, 1, vk::ImageUsageFlagBits::eColorAttachment, imageSharingMode,
			queueFamilyIndexCount,
			!queueFamilyIndices.empty() ? &queueFamilyIndices[0] : nullptr,
			swapChainSupport.Capabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, VK_TRUE));

		if (!swapchain_)
		{
			throw std::runtime_error(
				"Could not create vk::SwapchainKHR. Verify your hardware is supported, and your drivers are up-to-date.");
		}

		swapchainImages_ = deviceContext_.LogicalDevice->getSwapchainImagesKHR(*swapchain_);
		swapchainExtent_ = extent;

		CreateImageViews(surfaceFormat.format);
		CreateRenderPass(surfaceFormat.format);
	}

	void RecreateSwapchain(const vk::Extent2D swapchainExtent)
	{
		WaitIdle();
		DestroySwapchain();
		CreateSwapchain(swapchainExtent);
	}

	void CreateImageViews(const vk::Format imageFormat)
	{
		for (const auto image : swapchainImages_)
		{
			auto imageView = deviceContext_.LogicalDevice->createImageViewUnique(vk::ImageViewCreateInfo(
				{}, image,
				vk::ImageViewType::e2D, imageFormat,
				{},
				vk::ImageSubresourceRange(
					vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
			if (!imageView)
			{
				throw std::runtime_error("Could not create vk::ImageView for given swapChainImage.");
			}
			swapchainImageViews_.emplace_back(std::move(imageView));
		}
	}

	void CreateRenderPass(const vk::Format imageFormat)
	{
		vk::AttachmentDescription colorAttachment({}, imageFormat,
			vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR);
		vk::AttachmentReference colorAttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
		vk::AttachmentDescription depthAttachment({}, Util::FindSupportedFormat(deviceContext_,
			{
				vk::Format::eD32Sfloat,
				vk::Format::eD32SfloatS8Uint,
				vk::Format::eD24UnormS8Uint
			}, vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::
			eDepthStencilAttachment),
			vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, {},
			vk::ImageLayout::eDepthStencilAttachmentOptimal);
		vk::AttachmentReference depthAttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
		vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1,
			&colorAttachmentReference, {}, &depthAttachmentReference);
		vk::SubpassDependency subpassDependency(
			VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, {},
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
		const vk::AttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
		renderPass_ = deviceContext_.LogicalDevice->createRenderPassUnique(
			vk::RenderPassCreateInfo({}, 2, attachments, 1, &subpass, 1,
			                         &subpassDependency));

		if (!renderPass_)
		{
			throw std::runtime_error(
				"Could not create vk::RenderPass. Verify your hardware is supported, and your drivers are up-to-date.");
		}
	}

	void CreateGraphicsPipeline(const std::stringstream& vertexShaderSource,
	                                  const std::stringstream& fragmentShaderSource,
	                                  const vk::VertexInputBindingDescription& vertexInputBindingDescription,
	                                  const std::array<vk::VertexInputAttributeDescription, 3>&
	                                  vertexInputAttributeDescriptions, const float minDepth, const float maxDepth)
	{
		auto vertexShaderModule = createShaderModule_(vertexShaderSource);
		auto fragmentShaderModule = createShaderModule_(fragmentShaderSource);

		vk::PipelineShaderStageCreateInfo vertexShaderPipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eVertex, vertexShaderModule.get(), "main");

		vk::PipelineShaderStageCreateInfo fragmentShaderPipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eFragment, fragmentShaderModule.get(), "main");

		vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[] = {
			vertexShaderPipelineShaderStageCreateInfo, fragmentShaderPipelineShaderStageCreateInfo
		};
		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
			{}, 1, &vertexInputBindingDescription, static_cast<uint32_t>(vertexInputAttributeDescriptions.size()), &vertexInputAttributeDescriptions[0]);
		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
			{}, vk::PrimitiveTopology::eTriangleList);
		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapchainExtent_.width),
			static_cast<float>(swapchainExtent_.height), minDepth, maxDepth);
		vk::Rect2D scissor({ 0, 0 }, swapchainExtent_);
		vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo({}, 1, &viewport, 1, &scissor);
		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
			{}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack,
			vk::FrontFace::eCounterClockwise,
			VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f);
		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
			{}, vk::SampleCountFlagBits::e1, VK_FALSE);
		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo({}, VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f);
		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(VK_FALSE, {}, {}, {}, {}, {}, {},
			vk::ColorComponentFlagBits::eR | vk::
			ColorComponentFlagBits::eG | vk::
			ColorComponentFlagBits::eB | vk::
			ColorComponentFlagBits::eA);
		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
			{}, VK_FALSE, vk::LogicOp::eCopy, 1, &pipelineColorBlendAttachmentState);

		const auto rawDescriptorSetLayouts = vk::uniqueToRaw(descriptorSetLayouts_);
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, rawDescriptorSetLayouts.size(), &rawDescriptorSetLayouts[0]);
		graphicsPipelineLayout_ = deviceContext_.LogicalDevice->createPipelineLayoutUnique(pipelineLayoutCreateInfo);
		if (!graphicsPipelineLayout_)
		{
			throw std::runtime_error(
				"Could not create vk::PipelineLayout for the Vulkan graphics pipeline. Verify your hardware is supported, and your drivers are up-to-date.");
		}

		graphicsPipeline_ = deviceContext_.LogicalDevice->createGraphicsPipelineUnique(nullptr, {
			                                                                               {}, 2,
			                                                                               pipelineShaderStageCreateInfos,
			                                                                               &pipelineVertexInputStateCreateInfo,
			                                                                               &pipelineInputAssemblyStateCreateInfo,
			                                                                               nullptr,
			                                                                               &pipelineViewportStateCreateInfo,
			                                                                               &pipelineRasterizationStateCreateInfo,
			                                                                               &pipelineMultisampleStateCreateInfo,
			                                                                               &pipelineDepthStencilStateCreateInfo,
			                                                                               &pipelineColorBlendStateCreateInfo,
			                                                                               nullptr,
			                                                                               *graphicsPipelineLayout_,
			                                                                               *renderPass_
		                                                                               });
		if (!graphicsPipeline_)
		{
			throw std::runtime_error(
				"Could not create vk::Pipeline for the Vulkan graphics pipeline. Verify your hardware is supported, and your drivers are up-to-date.");
		}
	}

	void CreateFramebuffers()
	{
		for (vk::UniqueImageView& imageView : swapchainImageViews_)
		{
			std::array<vk::ImageView, 2> attachments = { imageView.get(), depthImage_->GetImageView().get() };
			auto framebuffer = deviceContext_.LogicalDevice->createFramebufferUnique({
				{}, *renderPass_, attachments.size(), &attachments[0],
				swapchainExtent_.width, swapchainExtent_.height, 1
			});
			if (!framebuffer)
			{
				throw std::runtime_error("Could not create vk::Framebuffer for given imageView.");
			}
			swapchainFramebuffers_.emplace_back(std::move(framebuffer));
		}
	}

	void CreateCommandPool()
	{
		auto queueFamilyIndices = findQueueFamilies_(*deviceContext_.PhysicalDevice);
		commandPool_ = deviceContext_.LogicalDevice->createCommandPoolUnique({
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			queueFamilyIndices.GraphicsFamily.value()
		});
		if (!commandPool_)
		{
			throw std::runtime_error(
				"Could not create vk::CommandPool. Verify your hardware is supported, and your drivers are up-to-date.");
		}
	}

	void CreateDepthBuffer()
	{
		const auto imageFormat = Util::FindSupportedFormat(deviceContext_,
		                                                   {
			                                                   vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
			                                                   vk::Format::eD24UnormS8Uint
		                                                   }, vk::ImageTiling::eOptimal,
		                                                   vk::FormatFeatureFlagBits::eDepthStencilAttachment);
		depthImage_.reset(new Image(deviceContext_, commandPool_,
		                            {swapchainExtent_.width, swapchainExtent_.height, 1}, imageFormat,
		                            vk::ImageUsageFlagBits::eDepthStencilAttachment,
		                            vk::MemoryPropertyFlagBits::eDeviceLocal));
		depthImage_->CreateImageView(imageFormat, vk::ImageAspectFlagBits::eDepth);
	}

	void CreateUniformBuffers(const size_t bufferSize)
	{
		uniformBuffers_.resize(swapchainImages_.size());

		for (size_t i = 0; i < swapchainImages_.size(); ++i)
		{
			uniformBuffers_[i].reset(new GenericBuffer(deviceContext_, commandPool_, bufferSize,
			                                           vk::BufferUsageFlagBits::eUniformBuffer,
			                                           vk::SharingMode::eExclusive,
			                                           vk::MemoryPropertyFlagBits::eHostVisible |
			                                           vk::MemoryPropertyFlagBits::eHostCoherent,
			                                           "VulkanContext::uniformBuffers_[" + std::to_string(i) + "]"));
		}
	}

	void CreateDescriptorPool()
	{
		const std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
			{
				vk::DescriptorType::eUniformBuffer,
				static_cast<uint32_t>(swapchainImages_.size())
			},
			{
				vk::DescriptorType::eCombinedImageSampler,
				static_cast<uint32_t>(swapchainImages_.size())
			}
		};
		descriptorPool_ = deviceContext_.LogicalDevice->createDescriptorPoolUnique({
			{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
			static_cast<uint32_t>(swapchainImages_.size()),
			static_cast<uint32_t>(descriptorPoolSizes.size()), &descriptorPoolSizes[0]
		});
	}

	/*
	 * TODO: Break out CommandBuffer creation, allow mesh uploading from VulkanRenderingEngine, and instead of having the user pass in a single ::Mesh,
	 * iterate through all of the active meshes, and render out each mesh for each vk::CommandBuffer's render pass.
	 */
	void CreateCommandBuffers()
	{
		const vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*commandPool_, vk::CommandBufferLevel::ePrimary,
		                                                              static_cast<uint32_t>(swapchainFramebuffers_.
			                                                              size()));
		commandBuffers_ = deviceContext_.LogicalDevice->allocateCommandBuffersUnique(commandBufferAllocateInfo);
		if (commandBuffers_.empty())
		{
			throw std::runtime_error(
				"Could not allocate vk::CommandBuffer(s). Verify your hardware is supported, and your drivers are up-to-date.");
		}
	}

	void BeginRenderPass(const uint32_t frameIndex)
	{
		auto& buffer = commandBuffers_[frameIndex];
		vk::CommandBufferBeginInfo commandBufferBeginInfo;
		buffer->reset({});
		buffer->begin(&commandBufferBeginInfo);
		const std::array<vk::ClearValue, 2> clearValues = {
			vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}), vk::ClearDepthStencilValue(1.0f, 0.0f)
		};
		vk::RenderPassBeginInfo renderPassBeginInfo(*renderPass_, swapchainFramebuffers_[frameIndex].get(),
		                                            {{0, 0}, swapchainExtent_}, clearValues.size(), &clearValues[0]);

		buffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_);
		
	}

	void EndRenderPass(const uint32_t frameIndex)
	{
		auto& buffer = commandBuffers_[frameIndex];
		buffer->endRenderPass();
		buffer->end();
	}
	
	void CreateSyncObjects(const size_t maxFramesInFlight)
	{
		maxFramesInFlight_ = maxFramesInFlight;
		const vk::SemaphoreCreateInfo semaphoreCreateInfo;
		const vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);

		for (size_t i = 0; i < maxFramesInFlight; i++)
		{
			vk::UniqueSemaphore imageAvailableSemaphore = deviceContext_.LogicalDevice->createSemaphoreUnique(semaphoreCreateInfo);
			if (!imageAvailableSemaphore)
			{
				throw std::runtime_error(
					"Could not create vk::Semaphore(s) for frame [" + std::to_string(i) +
					"] - imageAvailableSemaphore.");
			}
			imageAvailableSemaphores_.emplace_back(std::move(imageAvailableSemaphore));
			vk::UniqueSemaphore renderFinishedSemaphore = deviceContext_.LogicalDevice->createSemaphoreUnique(semaphoreCreateInfo);
			if (!renderFinishedSemaphore)
			{
				throw std::runtime_error(
					"Could not create vk::Semaphore(s) for frame [" + std::to_string(i) +
					"] - renderFinishedSemaphore.");
			}
			renderFinishedSemaphores_.emplace_back(std::move(renderFinishedSemaphore));
			vk::UniqueFence inFlightFence = deviceContext_.LogicalDevice->createFenceUnique(fenceCreateInfo);
			if (!inFlightFence)
			{
				throw std::runtime_error(
					"Could not create vk::Fence(s) for frame [" + std::to_string(i) + "] - inFlightFence.");
			}
			inFlightFences_.emplace_back(std::move(inFlightFence));
		}
	}

	void WaitIdle()
	{
		deviceContext_.LogicalDevice->waitIdle();
	}

	void WaitForFences()
	{
		const auto result = deviceContext_.LogicalDevice->waitForFences(*inFlightFences_[0], VK_TRUE, UINT64_MAX);
		
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Could not wait for Vulkan fences.");
		}
	}

	void UpdateUniformBuffer(const uint32_t index, const vk::DeviceSize offset, const vk::DeviceSize size, const void* data)
	{
		uniformBuffers_[index]->Fill(offset, size, data);
	}

	uint32_t AddDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
	{
		auto descriptorSetLayout = deviceContext_.LogicalDevice->createDescriptorSetLayoutUnique({
			{}, static_cast<uint32_t>(bindings.size()), &bindings[0]
		});
		descriptorSetLayouts_.emplace_back(std::move(descriptorSetLayout));

		return descriptorSetLayouts_.size() - 1;
	}

	vk::UniqueDescriptorSetLayout& GetDescriptorSetLayout(const uint32_t index)
	{
		return descriptorSetLayouts_[index];
	}

	uint32_t AddDescriptorSet(const uint32_t descriptorSetLayoutIndex)
	{
		auto& layout = GetDescriptorSetLayout(descriptorSetLayoutIndex);
		descriptorSets_.emplace_back(std::make_shared<DescriptorSet>(deviceContext_, descriptorPool_, maxFramesInFlight_, layout.get()));
		return descriptorSets_.size() - 1;
	}

	std::shared_ptr<DescriptorSet> GetDescriptorSet(const uint32_t index)
	{
		return descriptorSets_[index];
	}

	/*
	 * descriptorWrites example:
	 * {
	 *	{ nullptr, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &descriptorBufferInfo },
	 *	{ nullptr, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descriptorImageInfo }
	 * }
	 */
	void UpdateDescriptorSet(const uint32_t index, std::vector<vk::WriteDescriptorSet>& descriptorWrites)
	{
		DebugMessage("Scene::UpdateDescriptorSet(" + std::to_string(index) + ")");
		descriptorSets_[index]->Update(descriptorWrites);
	}

	vk::Result AcquireNextImage(const size_t currentFrame, uint32_t& imageIndex)
	{
		return deviceContext_.LogicalDevice->acquireNextImageKHR(swapchain_.get(), UINT64_MAX,
		                                                         imageAvailableSemaphores_[currentFrame].get(), nullptr,
		                                                         &imageIndex);
	}

	void WaitForRenderFinished(const size_t frameIndex, const uint32_t imageIndex)
	{
		deviceContext_.LogicalDevice->waitForFences(1, &inFlightFences_[frameIndex].get(), true, UINT64_MAX);
		deviceContext_.LogicalDevice->resetFences(inFlightFences_[frameIndex].get());
	}

	void SubmitFrame(const size_t frameIndex, const uint32_t imageIndex)
	{
		vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores_[frameIndex].get() };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores_[frameIndex].get() };
		vk::CommandBuffer commandBuffers[] = { commandBuffers_[imageIndex].get() };
		const vk::SubmitInfo submitInfo(1, waitSemaphores, waitStages, 1, commandBuffers, 1, signalSemaphores);
		deviceContext_.GraphicsQueue->submit(submitInfo, inFlightFences_[frameIndex].get());
		presentSwapchain_(submitInfo.pSignalSemaphores[0], imageIndex);
	}

	void DestroySwapchain()
	{
		swapchainFramebuffers_.clear();
		commandBuffers_.clear();

		graphicsPipeline_.reset();
		graphicsPipelineLayout_.reset();
		renderPass_.reset();

		uniformBuffers_.clear();
		
		// for (size_t i = 0; i < uniformBuffers_.size(); i++)
		// {
		// 	uniformBuffers_[i].reset();
		// }

		// TODO: [zpuls 2020-07-31T18:16] Add better error handling to VulkanContext::DestroySwapchain(). \
											Namely, not attempting to delete/destroy objects that don't exist.

		descriptorPool_.reset();

		swapchainImageViews_.clear();

		swapchain_.reset();
	}

	void UpdateMesh(const uint32_t frameIndex, std::shared_ptr<Mesh> mesh, std::shared_ptr<Texture> texture)
	{
		auto descriptorBufferInfo = mesh->GetUniformBuffer(frameIndex)->GenerateDescriptorBufferInfo();
		auto descriptorImageInfo = texture->GenerateDescriptorImageInfo();
		std::vector<vk::WriteDescriptorSet> descriptorWrites = {
			{nullptr, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &descriptorBufferInfo},
			{nullptr, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descriptorImageInfo}
		};
		descriptorSets_[mesh->GetDescriptorSetIndex()]->Update(descriptorWrites);
	}

	void UpdateParticleEffect(const uint32_t frameIndex, std::shared_ptr<ParticleEffect> particleEffect, std::shared_ptr<Texture> texture)
	{
		auto descriptorBufferInfo = particleEffect->GetUniformBuffer(frameIndex)->GenerateDescriptorBufferInfo();
		auto descriptorImageInfo = texture->GenerateDescriptorImageInfo();
		std::vector<vk::WriteDescriptorSet> descriptorWrites = {
			{ nullptr, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &descriptorBufferInfo },
			{ nullptr, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descriptorImageInfo }
		};
		descriptorSets_[particleEffect->GetDescriptorSetIndex()]->Update(descriptorWrites);
	}

	vk::UniqueCommandBuffer& GetCommandBuffer(const uint32_t index)
	{
		return commandBuffers_[index];
	}

	vk::UniquePipelineLayout& GetGraphicsPipelineLayout()
	{
		return graphicsPipelineLayout_;
	}
private:
	vk::UniqueInstance instance_;
	std::vector<const char*> enabledLayers_;
	vk::UniqueSurfaceKHR surface_;
	VulkanDeviceContext deviceContext_;
	vk::UniqueSwapchainKHR swapchain_;
	vk::Extent2D swapchainExtent_;
	std::vector<vk::Image> swapchainImages_;
	std::vector<vk::UniqueImageView> swapchainImageViews_;
	std::shared_ptr<Image> depthImage_;
	vk::UniqueRenderPass renderPass_;
	vk::UniqueDescriptorPool descriptorPool_;
	std::vector<vk::UniqueDescriptorSetLayout> descriptorSetLayouts_;
	std::vector<std::shared_ptr<DescriptorSet>> descriptorSets_;
	vk::UniquePipelineLayout graphicsPipelineLayout_;
	vk::UniquePipeline graphicsPipeline_;
	std::vector<vk::UniqueFramebuffer> swapchainFramebuffers_;
	vk::UniqueCommandPool commandPool_;
	std::vector<vk::UniqueCommandBuffer> commandBuffers_;
	std::vector<std::shared_ptr<GenericBuffer>> uniformBuffers_;
	std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;
	std::vector<vk::UniqueSemaphore> renderFinishedSemaphores_;
	std::vector<vk::UniqueFence> inFlightFences_;
	size_t maxFramesInFlight_ = 0;

	// TODO: [zpuls 2020-07-27T01:40 CDT] Break out mesh/texture/buffer allocation/deallocation into its own class, possibly set up a new memory/resource management schema? Not sure yet.

	inline static const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

	bool isVulkanPhysicalDeviceSuitable_(const vk::PhysicalDevice physicalDevice)
	{
		auto indices = findQueueFamilies_(physicalDevice);
		const auto extensionsSupported = checkDeviceExtensionSupport_(physicalDevice);
		const auto featuresSupported = checkDeviceFeatureSupport_(physicalDevice);

		auto swapChainAdequate = false;
		if (extensionsSupported)
		{
			const auto swapChainSupport = querySwapChainSupport_(physicalDevice);
			swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
		}

		return indices.IsComplete() && extensionsSupported && featuresSupported && swapChainAdequate;
	}

	SwapChainSupportDetails querySwapChainSupport_(const vk::PhysicalDevice& physicalDevice) const
	{
		return {
			physicalDevice.getSurfaceCapabilitiesKHR(surface_.get()),
			physicalDevice.getSurfaceFormatsKHR(surface_.get()),
			physicalDevice.getSurfacePresentModesKHR(surface_.get()),
		};
	}

	QueueFamilyIndices findQueueFamilies_(const vk::PhysicalDevice& physicalDevice)
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

			if (queueFamily.queueCount > 0 && physicalDevice.getSurfaceSupportKHR(i, surface_.get()))
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

	bool checkDeviceExtensionSupport_(const vk::PhysicalDevice& physicalDevice)
	{
		auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
		std::set<std::string> requiredExtensions(REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	bool checkDeviceFeatureSupport_(const vk::PhysicalDevice& physicalDevice)
	{
		auto availableFeatures = physicalDevice.getFeatures();
		return availableFeatures.samplerAnisotropy;
	}

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat_(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
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

	vk::PresentModeKHR chooseSwapPresentMode_(const std::vector<vk::PresentModeKHR>& availablePresentModes)
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

	vk::Extent2D chooseSwapExtent_(const vk::SurfaceCapabilitiesKHR& capabilities, const vk::Extent2D& requestedExtent)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}

		auto actualExtent = requestedExtent;
		actualExtent.width = std::max(capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, requestedExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, requestedExtent.height));

		return actualExtent;
	}

	vk::UniqueShaderModule createShaderModule_(const std::stringstream& code) const
	{
		const auto codeString = code.str();
		return deviceContext_.LogicalDevice->createShaderModuleUnique({
			{}, codeString.length(), reinterpret_cast<const uint32_t*>(&codeString[0])
		});
	}
	
	void presentSwapchain_(vk::Semaphore signalSemaphore, const uint32_t imageIndex)
	{
		const vk::PresentInfoKHR presentInfo(1, &signalSemaphore, 1, &swapchain_.get(), &imageIndex);
		try
		{
			deviceContext_.PresentQueue->presentKHR(presentInfo);
		}
		catch (vk::OutOfDateKHRError&)
		{
			throw SwapchainOutOfDateException();
		}
		catch (...)
		{
			throw PresentSwapchainUnknownException();
		}
	}
};
