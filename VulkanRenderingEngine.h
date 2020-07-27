#pragma once

#include "stdafx.h"
#include "Window.h"

class VulkanRenderingEngine
{
public:
	VulkanRenderingEngine(Window& window)
	{
		createVulkanInstance_(window);
		selectVulkanPhysicalDevice_();
		createVulkanSurface_(window);
		createVulkanLogicalDevice_();
		auto windowSize = window.GetDimensions();
		createVulkanSwapchain_(windowSize[0], windowSize[1]);
	}

private:
	vk::UniqueInstance instance_;
	vk::PhysicalDevice physicalDevice_;
	vk::UniqueSurfaceKHR surface_;
	vk::UniqueDevice logicalDevice_;
	size_t graphicsQueueFamilyIndex_;
	size_t presentQueueFamilyIndex_;
	std::vector<uint32_t> queueFamilyIndices_;
	vk::UniqueSwapchainKHR swapchain_;
	std::vector<vk::Image> swapchainImages_;
	std::vector<vk::UniqueImageView> swapchainImageViews_;
	
	vk::Queue graphicsQueue_;
	vk::Queue presentQueue_;

	static inline const char* ENGINE_NAME = "BZEngine";
	static inline const uint32_t ENGINE_VERSION = VK_MAKE_VERSION(0, 0, 2);
	static inline const std::vector<const char*> ENGINE_LAYERS = {"VK_LAYER_LUNARG_standard_validation"};

	static inline const std::vector<const char*> ENGINE_REQUIRED_DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	static inline const uint32_t ENGINE_DEFAULT_NUM_SWAPCHAIN_IMAGES = 2;

	void createVulkanInstance_(Window& window)
	{
		vk::ApplicationInfo applicationInfo(window.GetTitle().c_str(), VK_MAKE_VERSION(0, 0, 0), ENGINE_NAME,
		                                    ENGINE_VERSION, VK_API_VERSION_1_1);
		auto requiredExtensions = window.GetRequiredExtensions();
		instance_ = vk::createInstanceUnique({
			{}, &applicationInfo, ENGINE_LAYERS.size(), &ENGINE_LAYERS[0], requiredExtensions.size(),
			&requiredExtensions[0]
		});

		auto messenger = instance_->createDebugUtilsMessengerEXTUnique(
			vk::DebugUtilsMessengerCreateInfoEXT{
				{},
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
				debugCallback_
			},
			nullptr, vk::DispatchLoaderDynamic{*instance_});
	}

	void selectVulkanPhysicalDevice_()
	{
		auto physicalDevices = instance_->enumeratePhysicalDevices();
		physicalDevice_ = physicalDevices[std::distance(physicalDevices.begin(),
		                                                std::find_if(physicalDevices.begin(), physicalDevices.end(),
		                                                             [](const vk::PhysicalDevice& physicalDevice)
		                                                             {
			                                                             return strstr(
				                                                             physicalDevice.getProperties().deviceName,
				                                                             "GeForce");
		                                                             }))];
	}

	void createVulkanSurface_(Window& window)
	{
		VkSurfaceKHR surfaceTmp;
		glfwCreateWindowSurface(*instance_, window.GetHandle(), nullptr, &surfaceTmp);

		surface_ = vk::UniqueSurfaceKHR(surfaceTmp, *instance_);
	}

	std::vector<vk::DeviceQueueCreateInfo> generateVulkanQueueCreateInfos_()
	{
		auto queueFamilyProperties = physicalDevice_.getQueueFamilyProperties();

		graphicsQueueFamilyIndex_ = std::distance(queueFamilyProperties.begin(),
		                                          std::find_if(queueFamilyProperties.begin(),
		                                                       queueFamilyProperties.end(),
		                                                       [](vk::QueueFamilyProperties const& qfp)
		                                                       {
			                                                       return qfp.queueFlags & vk::QueueFlagBits::
				                                                       eGraphics;
		                                                       }));

		presentQueueFamilyIndex_ = 0u;
		for (size_t i = 0; i < queueFamilyProperties.size(); i++)
		{
			if (physicalDevice_.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface_.get()))
			{
				presentQueueFamilyIndex_ = i;
			}
		}

		std::set<size_t> uniqueQueueFamilyIndices = {graphicsQueueFamilyIndex_, presentQueueFamilyIndex_};

		queueFamilyIndices_ = {
			uniqueQueueFamilyIndices.begin(),
			uniqueQueueFamilyIndices.end()
		};

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

		float queuePriority = 0.0f;
		for (int queueFamilyIndex : uniqueQueueFamilyIndices)
		{
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{
				vk::DeviceQueueCreateFlags(),
				static_cast<uint32_t>(queueFamilyIndex), 1, &queuePriority
			});
		}

		return queueCreateInfos;
	}

	void createVulkanLogicalDevice_()
	{
		auto queueCreateInfos = generateVulkanQueueCreateInfos_();
		logicalDevice_ = physicalDevice_.createDeviceUnique(
			vk::DeviceCreateInfo(vk::DeviceCreateFlags(), queueCreateInfos.size(),
			                     queueCreateInfos.data(), 0, nullptr, ENGINE_REQUIRED_DEVICE_EXTENSIONS.size(),
			                     &ENGINE_REQUIRED_DEVICE_EXTENSIONS[0]));
	}

	void createVulkanSwapchain_(int width, int height)
	{
		auto capabilities = physicalDevice_.getSurfaceCapabilitiesKHR(*surface_);
		auto formats = physicalDevice_.getSurfaceFormatsKHR(*surface_);

		auto format = vk::Format::eB8G8R8A8Unorm;
		auto extent = vk::Extent2D{width, height};

		struct SM
		{
			vk::SharingMode sharingMode;
			uint32_t familyIndicesCount;
			uint32_t* familyIndicesDataPtr;
		} sharingModeUtil{
				(graphicsQueueFamilyIndex_ != presentQueueFamilyIndex_)
					? SM{vk::SharingMode::eConcurrent, 2u, &queueFamilyIndices_[0]}
					: SM{vk::SharingMode::eExclusive, 0u, static_cast<uint32_t*>(nullptr)}
		};
		
		vk::SwapchainCreateInfoKHR swapChainCreateInfo({}, surface_.get(), ENGINE_DEFAULT_NUM_SWAPCHAIN_IMAGES, format,
		                                               vk::ColorSpaceKHR::eSrgbNonlinear, extent, 1,
		                                               vk::ImageUsageFlagBits::eColorAttachment,
		                                               sharingModeUtil.sharingMode, sharingModeUtil.familyIndicesCount,
		                                               sharingModeUtil.familyIndicesDataPtr,
		                                               vk::SurfaceTransformFlagBitsKHR::eIdentity,
		                                               vk::CompositeAlphaFlagBitsKHR::eOpaque,
		                                               vk::PresentModeKHR::eFifo, true, nullptr);

		swapchain_ = logicalDevice_->createSwapchainKHRUnique(swapChainCreateInfo);

		swapchainImages_ = logicalDevice_->getSwapchainImagesKHR(swapchain_.get());

		swapchainImageViews_.reserve(swapchainImages_.size());
		for (auto image : swapchainImages_)
		{
			vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image,
			                                            vk::ImageViewType::e2D, format,
			                                            vk::ComponentMapping{
				                                            vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
				                                            vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA
			                                            },
			                                            vk::ImageSubresourceRange{
				                                            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
			                                            });
			swapchainImageViews_.push_back(logicalDevice_->createImageViewUnique(imageViewCreateInfo));
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback_(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                                     void* pUserData)
	{
		auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		tm nowTm;
		localtime_s(&nowTm, &now);
		const auto timeString = std::put_time(&nowTm, "%Y-%m-%d %X");
		std::cerr << "[DEBUG] [" << timeString << "]: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}
};
