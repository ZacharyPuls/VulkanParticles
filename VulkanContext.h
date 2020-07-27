#pragma once

#include "stdafx.h"
#include <set>
#include <optional>
#include "Image.h"
#include "VulkanDeviceContext.h"
#include "Mesh.h"
#include "VulkanParticlesException.h"
#include "GenericBuffer.h"

class VulkanContext
{
public:

	VulkanContext(const std::vector<const char*>& enabledLayers, const std::vector<const char*>& extensions,
		const std::string& appName, const uint32_t appVersion, const std::string& engineName,
		const uint32_t engineVersion) : enabledLayers_(enabledLayers)
	{
		const uint32_t enabledLayerCount = enabledLayers_.size();
		uint32_t enabledExtensionsCount = extensions.size();
		auto enabledExtensions = &extensions[0];

		const auto applicationInfo = vk::ApplicationInfo(appName.c_str(), appVersion, engineName.c_str(), engineVersion,
			VK_API_VERSION_1_1);
		instance_ = std::make_shared<vk::Instance>(createInstance(vk::InstanceCreateInfo({}, &applicationInfo, enabledLayerCount, &enabledLayers_[0],
			enabledExtensionsCount, enabledExtensions)));
	}
	
	~VulkanContext()
	{
		DebugMessage("Cleaning up VulkanParticles::VulkanContext.");
		DestroySwapchain();

		depthImage_.reset();
		
		deviceContext_.LogicalDevice->destroyDescriptorSetLayout(*descriptorSetLayout_);

		for (size_t i = 0; i < maxFramesInFlight_; i++)
		{
			deviceContext_.LogicalDevice->destroySemaphore(renderFinishedSemaphores_[i]);
			deviceContext_.LogicalDevice->destroySemaphore(imageAvailableSemaphores_[i]);
			deviceContext_.LogicalDevice->destroyFence(inFlightFences_[i]);
		}

		deviceContext_.LogicalDevice->destroyCommandPool(*commandPool_);
		deviceContext_.LogicalDevice->destroy();

		instance_->destroySurfaceKHR(*surface_);
		instance_->destroy();
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
	
	std::shared_ptr<vk::Instance> GetInstance() const
	{
		return instance_;
	}

	std::vector<const char*> GetEnabledLayers() const
	{
		return enabledLayers_;
	}

	std::shared_ptr<vk::SurfaceKHR> GetSurface() const
	{
		return surface_;
	}

	VulkanDeviceContext GetDeviceContext() const
	{
		return deviceContext_;
	}

	std::shared_ptr<vk::Queue> GetGraphicsQueue() const
	{
		return deviceContext_.GraphicsQueue;
	}

	std::shared_ptr<vk::Queue> GetPresentQueue() const
	{
		return deviceContext_.PresentQueue;
	}

	std::shared_ptr<vk::SwapchainKHR> GetSwapchain() const
	{
		return swapchain_;
	}

	vk::Format GetSwapchainImageFormat() const
	{
		return swapchainImageFormat_;
	}

	vk::Extent2D GetSwapchainExtent() const
	{
		return swapchainExtent_;
	}

	std::vector<vk::Image> GetSwapchainImages() const
	{
		return swapchainImages_;
	}

	std::vector<vk::ImageView> GetSwapchainImageViews() const
	{
		return swapchainImageViews_;
	}

	std::shared_ptr<Image> GetDepthImage() const
	{
		return depthImage_;
	}

	std::shared_ptr<vk::RenderPass> GetRenderPass() const
	{
		return renderPass_;
	}

	std::shared_ptr<vk::DescriptorSetLayout> GetDescriptorSetLayout() const
	{
		return descriptorSetLayout_;
	}

	std::shared_ptr<vk::DescriptorPool> GetDescriptorPool() const
	{
		return descriptorPool_;
	}

	std::vector<vk::DescriptorSet> GetDescriptorSets() const
	{
		return descriptorSets_;
	}

	std::shared_ptr<vk::PipelineLayout> GetGraphicsPipelineLayout() const
	{
		return graphicsPipelineLayout_;
	}

	std::shared_ptr<vk::Pipeline> GetGraphicsPipeline() const
	{
		return graphicsPipeline_;
	}

	std::vector<vk::Framebuffer> GetSwapchainFramebuffers() const
	{
		return swapchainFramebuffers_;
	}

	std::shared_ptr<vk::CommandPool> GetCommandPool() const
	{
		return commandPool_;
	}

	std::vector<vk::CommandBuffer> GetCommandBuffers() const
	{
		return commandBuffers_;
	}
	
	void CreateSurface(GLFWwindow* appWindow)
	{
		VkSurfaceKHR surfaceHandle;
		if (glfwCreateWindowSurface(*instance_, appWindow, nullptr, &surfaceHandle) != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create vk::SurfaceKHR from appWindow.");
		}
		surface_ = std::make_shared<vk::SurfaceKHR>(surfaceHandle);
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
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo({}, queueFamily, 1, &queuePriority));
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
		
		swapchain_ = std::make_shared<vk::SwapchainKHR>(deviceContext_.LogicalDevice->createSwapchainKHR(vk::SwapchainCreateInfoKHR({}, *surface_,
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
			presentMode, VK_TRUE)));

		if (!swapchain_)
		{
			throw std::runtime_error(
				"Could not create vk::SwapchainKHR. Verify your hardware is supported, and your drivers are up-to-date.");
		}

		swapchainImages_ = deviceContext_.LogicalDevice->getSwapchainImagesKHR(*swapchain_);
		swapchainImageFormat_ = surfaceFormat.format;
		swapchainExtent_ = extent;
	}

	void CreateImageViews()
	{
		for (const auto image : swapchainImages_)
		{
			auto imageView = deviceContext_.LogicalDevice->createImageView(vk::ImageViewCreateInfo(
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

	void CreateRenderPass()
	{
		vk::AttachmentDescription colorAttachment({}, swapchainImageFormat_,
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
		renderPass_ = std::make_shared<vk::RenderPass>(deviceContext_.LogicalDevice->createRenderPass(vk::RenderPassCreateInfo({}, 2, attachments, 1,
			&subpass, 1, &subpassDependency)));

		if (!renderPass_)
		{
			throw std::runtime_error(
				"Could not create vk::RenderPass. Verify your hardware is supported, and your drivers are up-to-date.");
		}
	}

	void CreateDescriptorSetLayout()
	{
		vk::DescriptorSetLayoutBinding uniformBufferBinding(0, vk::DescriptorType::eUniformBuffer, 1,
			vk::ShaderStageFlagBits::eVertex);
		vk::DescriptorSetLayoutBinding uniformSamplerBinding(1, vk::DescriptorType::eCombinedImageSampler, 1,
			vk::ShaderStageFlagBits::eFragment);
		const std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
			uniformSamplerBinding, uniformBufferBinding
		};
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
			{}, static_cast<uint32_t>(descriptorSetLayoutBindings.size()), &descriptorSetLayoutBindings[0]);
		descriptorSetLayout_ = std::make_shared<vk::DescriptorSetLayout>(deviceContext_.LogicalDevice->createDescriptorSetLayout(descriptorSetLayoutCreateInfo));
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
			{}, vk::ShaderStageFlagBits::eVertex, vertexShaderModule, "main");

		vk::PipelineShaderStageCreateInfo fragmentShaderPipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eFragment, fragmentShaderModule, "main");

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

		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, 1, descriptorSetLayout_.get());
		graphicsPipelineLayout_ = std::make_shared<vk::PipelineLayout>(deviceContext_.LogicalDevice->createPipelineLayout(pipelineLayoutCreateInfo));
		if (!graphicsPipelineLayout_)
		{
			throw std::runtime_error(
				"Could not create vk::PipelineLayout for the Vulkan graphics pipeline. Verify your hardware is supported, and your drivers are up-to-date.");
		}

		graphicsPipeline_ = std::make_shared<vk::Pipeline>(deviceContext_.LogicalDevice->createGraphicsPipeline(nullptr, {
																	   {}, 2, pipelineShaderStageCreateInfos,
																	   &pipelineVertexInputStateCreateInfo,
																	   &pipelineInputAssemblyStateCreateInfo, nullptr,
																	   &pipelineViewportStateCreateInfo,
																	   &pipelineRasterizationStateCreateInfo,
																	   &pipelineMultisampleStateCreateInfo, &pipelineDepthStencilStateCreateInfo,
																	   &pipelineColorBlendStateCreateInfo, nullptr,
																	   *graphicsPipelineLayout_, *renderPass_
			}));
		if (!graphicsPipeline_)
		{
			throw std::runtime_error(
				"Could not create vk::Pipeline for the Vulkan graphics pipeline. Verify your hardware is supported, and your drivers are up-to-date.");
		}

		deviceContext_.LogicalDevice->destroyShaderModule(vertexShaderModule);
		deviceContext_.LogicalDevice->destroyShaderModule(fragmentShaderModule);
	}

	void CreateFramebuffers()
	{
		for (const auto imageView : swapchainImageViews_)
		{
			const std::array<vk::ImageView, 2> attachments = { imageView, depthImage_->GetImageView() };
			vk::FramebufferCreateInfo framebufferCreateInfo({}, *renderPass_, attachments.size(), &attachments[0], swapchainExtent_.width,
				swapchainExtent_.height, 1);
			auto framebuffer = deviceContext_.LogicalDevice->createFramebuffer(framebufferCreateInfo);
			if (!framebuffer)
			{
				throw std::runtime_error("Could not create vk::Framebuffer for given imageView.");
			}
			swapchainFramebuffers_.push_back(framebuffer);
		}
	}

	void CreateCommandPool()
	{
		auto queueFamilyIndices = findQueueFamilies_(*deviceContext_.PhysicalDevice);
		const vk::CommandPoolCreateInfo commandPoolCreateInfo({}, queueFamilyIndices.GraphicsFamily.value());
		commandPool_ = std::make_shared<vk::CommandPool>(deviceContext_.LogicalDevice->createCommandPool(commandPoolCreateInfo));
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
			uniformBuffers_[i].reset(new GenericBuffer(deviceContext_, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, {},
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::
				eHostCoherent));
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
		descriptorPool_ = std::make_shared<vk::DescriptorPool>(deviceContext_.LogicalDevice->createDescriptorPool({
			{},
			static_cast<uint32_t>(swapchainImages_
				.size()),
			static_cast<uint32_t>(
				descriptorPoolSizes.size()),
			&descriptorPoolSizes[0]
		}));
	}

	/*
	 * TODO: Break out DescriptorSet creation, allow texture uploading from VulkanRenderingEngine, and instead of having the user pass in a single vk::DescriptorImageInfo,
	 * iterate through all of the active textures, and build a DescriptorBuffer for each ::Image.
	 */
	void CreateDescriptorSets(const size_t bufferSize, const vk::DescriptorImageInfo descriptorImageInfo)
	{
		std::vector<vk::DescriptorSetLayout>
			descriptorSetLayoutTemplates(swapchainImages_.size(), *descriptorSetLayout_);
		descriptorSets_ = deviceContext_.LogicalDevice->allocateDescriptorSets({
			*descriptorPool_, static_cast<uint32_t>(swapchainImages_.size()),
			&descriptorSetLayoutTemplates[0]
		});
		for (size_t i = 0; i < swapchainImages_.size(); ++i)
		{
			vk::DescriptorBufferInfo descriptorBufferInfo(uniformBuffers_[i]->GetHandle(), 0, bufferSize);
			deviceContext_.LogicalDevice->updateDescriptorSets(
				{
					{
						descriptorSets_[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, {},
						&descriptorBufferInfo
					},
					{
						descriptorSets_[i], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler,
						&descriptorImageInfo
					}
				}, nullptr);
		}
	}

	/*
	 * TODO: Break out CommandBuffer creation, allow mesh uploading from VulkanRenderingEngine, and instead of having the user pass in a single ::Mesh,
	 * iterate through all of the active meshes, and render out each mesh for each vk::CommandBuffer's render pass.
	 */
	void CreateCommandBuffers(const std::shared_ptr<Mesh>& mesh)
	{
		const vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*commandPool_, vk::CommandBufferLevel::ePrimary,
			static_cast<uint32_t>(swapchainFramebuffers_.
				size()));
		commandBuffers_ = deviceContext_.LogicalDevice->allocateCommandBuffers(commandBufferAllocateInfo);
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
			const std::array<vk::ClearValue, 2> clearValues = { vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}), vk::ClearDepthStencilValue(1.0f, 0.0f) };
			vk::RenderPassBeginInfo renderPassBeginInfo(*renderPass_, swapchainFramebuffers_[i],
				{ {0, 0}, swapchainExtent_ }, clearValues.size(), &clearValues[0]);

			buffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
			buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_);
			mesh->BindMeshData(buffer);
			/*particleEffect_->BindMeshData(buffer);*/
			buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipelineLayout_, 0, 1,
				&descriptorSets_[i], 0, nullptr);
			mesh->Draw(buffer);
			buffer.endRenderPass();
			buffer.end();
		}
	}

	void CreateSyncObjects(const size_t maxFramesInFlight)
	{
		maxFramesInFlight_ = maxFramesInFlight;
		const vk::SemaphoreCreateInfo semaphoreCreateInfo;
		const vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);

		for (size_t i = 0; i < maxFramesInFlight; i++)
		{
			auto imageAvailableSemaphore = deviceContext_.LogicalDevice->createSemaphore(semaphoreCreateInfo);
			if (!imageAvailableSemaphore)
			{
				throw std::runtime_error(
					"Could not create vk::Semaphore(s) for frame [" + std::to_string(i) +
					"] - imageAvailableSemaphore.");
			}
			imageAvailableSemaphores_.push_back(imageAvailableSemaphore);
			auto renderFinishedSemaphore = deviceContext_.LogicalDevice->createSemaphore(semaphoreCreateInfo);
			if (!renderFinishedSemaphore)
			{
				throw std::runtime_error(
					"Could not create vk::Semaphore(s) for frame [" + std::to_string(i) +
					"] - renderFinishedSemaphore.");
			}
			renderFinishedSemaphores_.push_back(renderFinishedSemaphore);
			auto inFlightFence = deviceContext_.LogicalDevice->createFence(fenceCreateInfo);
			if (!inFlightFence)
			{
				throw std::runtime_error(
					"Could not create vk::Fence(s) for frame [" + std::to_string(i) + "] - inFlightFence.");
			}
			inFlightFences_.push_back(inFlightFence);
		}
	}

	void WaitIdle()
	{
		deviceContext_.LogicalDevice->waitIdle();
	}

	void WaitForFences()
	{
		const auto result = deviceContext_.LogicalDevice->waitForFences(inFlightFences_, VK_TRUE, UINT64_MAX);
		
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Could not wait for Vulkan fences.");
		}
	}

	void UpdateUniformBuffer(const uint32_t index, const vk::DeviceSize offset, const vk::DeviceSize size, const void* data)
	{
		uniformBuffers_[index]->Fill(offset, size, data);
	}

	vk::Result AcquireNextImage(const size_t currentFrame, uint32_t& imageIndex)
	{
		return deviceContext_.LogicalDevice->acquireNextImageKHR(*swapchain_, UINT64_MAX,
		                                                         imageAvailableSemaphores_[currentFrame], nullptr,
		                                                         &imageIndex);
	}

	void SubmitFrame(const size_t frameIndex, const uint32_t imageIndex)
	{
		vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores_[frameIndex] };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores_[frameIndex] };
		const vk::SubmitInfo submitInfo(1, waitSemaphores, waitStages, 1, &commandBuffers_[imageIndex], 1,
			signalSemaphores);

		deviceContext_.LogicalDevice->resetFences(inFlightFences_[frameIndex]);
		deviceContext_.GraphicsQueue->submit(submitInfo, inFlightFences_[frameIndex]);

		presentSwapchain_(signalSemaphores, imageIndex);
	}

	void DestroySwapchain()
	{
		for (const auto framebuffer : swapchainFramebuffers_)
		{
			deviceContext_.LogicalDevice->destroyFramebuffer(framebuffer);
		}
		swapchainFramebuffers_.clear();

		deviceContext_.LogicalDevice->freeCommandBuffers(*commandPool_, commandBuffers_);
		commandBuffers_.clear();

		deviceContext_.LogicalDevice->destroyPipeline(*graphicsPipeline_);
		deviceContext_.LogicalDevice->destroyPipelineLayout(*graphicsPipelineLayout_);
		deviceContext_.LogicalDevice->destroyRenderPass(*renderPass_);

		uniformBuffers_.clear();
		
		// for (size_t i = 0; i < uniformBuffers_.size(); i++)
		// {
		// 	uniformBuffers_[i].reset();
		// }

		deviceContext_.LogicalDevice->destroyDescriptorPool(*descriptorPool_);

		for (const auto imageView : swapchainImageViews_)
		{
			deviceContext_.LogicalDevice->destroyImageView(imageView);
		}
		swapchainImageViews_.clear();

		deviceContext_.LogicalDevice->destroySwapchainKHR(*swapchain_);
	}
private:
	std::shared_ptr<vk::Instance> instance_;
	const std::vector<const char*>& enabledLayers_;
	std::shared_ptr<vk::SurfaceKHR> surface_;
	VulkanDeviceContext deviceContext_;
	std::shared_ptr<vk::SwapchainKHR> swapchain_;
	vk::Format swapchainImageFormat_;
	vk::Extent2D swapchainExtent_;
	std::vector<vk::Image> swapchainImages_;
	std::vector<vk::ImageView> swapchainImageViews_;
	std::shared_ptr<Image> depthImage_;
	std::shared_ptr<vk::RenderPass> renderPass_;
	std::shared_ptr<vk::DescriptorSetLayout> descriptorSetLayout_;
	std::shared_ptr<vk::DescriptorPool> descriptorPool_;
	std::vector<vk::DescriptorSet> descriptorSets_;
	std::shared_ptr<vk::PipelineLayout> graphicsPipelineLayout_;
	std::shared_ptr<vk::Pipeline> graphicsPipeline_;
	std::vector<vk::Framebuffer> swapchainFramebuffers_;
	std::shared_ptr<vk::CommandPool> commandPool_;
	std::vector<vk::CommandBuffer> commandBuffers_;
	std::vector<std::unique_ptr<GenericBuffer>> uniformBuffers_;
	std::vector<vk::Semaphore> imageAvailableSemaphores_;
	std::vector<vk::Semaphore> renderFinishedSemaphores_;
	std::vector<vk::Fence> inFlightFences_;
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
			physicalDevice.getSurfaceCapabilitiesKHR(*surface_),
			physicalDevice.getSurfaceFormatsKHR(*surface_),
			physicalDevice.getSurfacePresentModesKHR(*surface_),
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

			if (queueFamily.queueCount > 0 && physicalDevice.getSurfaceSupportKHR(i, *surface_))
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

	vk::ShaderModule createShaderModule_(const std::stringstream& code) const
	{
		const auto codeString = code.str();
		return deviceContext_.LogicalDevice->createShaderModule(vk::ShaderModuleCreateInfo(
			{}, codeString.length(), reinterpret_cast<const uint32_t*>(&codeString[0])));
	}
	
	void presentSwapchain_(const vk::Semaphore signalSemaphores[], const uint32_t imageIndex)
	{
		const vk::SwapchainKHR swapchains[] = { *swapchain_ };
		const vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, swapchains, &imageIndex);
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
