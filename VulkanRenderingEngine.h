#pragma once

#include "ParticleEffect.h"
#include "Scene.h"
#include "stdafx.h"
#include "VulkanContext.h"

class VulkanRenderingEngine
{
public:
	VulkanRenderingEngine(std::shared_ptr<VulkanContext> context, const uint32_t maxFramesInFlight)
		: context_(context),
		  maxFramesInFlight_(maxFramesInFlight)
	{
	}

	uint32_t BeginFrame(const vk::Extent2D currentSwapchainExtent)
	{
		context_->WaitForFences();

		uint32_t imageIndex;
		const auto result = context_->AcquireNextImage(currentFrame_, imageIndex);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			context_->RecreateSwapchain(currentSwapchainExtent);
		}

		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Could not acquire Vulkan swapchain image.");
		}

		context_->BeginRenderPass(currentFrame_);
		
		return imageIndex;
	}
	
	void RenderScene(std::shared_ptr<Scene> scene, const uint32_t imageIndex, const vk::Extent2D currentSwapchainExtent, const float deltaTime)
	{
		scene->SetSwapchainExtent(context_->GetSwapchainExtent());

		UpdateSceneMeshes(scene, deltaTime);
		RenderSceneObjects(scene);
		
		try
		{
			context_->WaitForRenderFinished(currentFrame_, imageIndex);
			context_->EndRenderPass(currentFrame_);
			context_->SubmitFrame(currentFrame_, imageIndex);
		}
		catch (VulkanContext::SwapchainOutOfDateException&)
		{
			// framebufferResized = false;
			context_->RecreateSwapchain(currentSwapchainExtent);
		}
		catch (VulkanContext::PresentSwapchainUnknownException&)
		{
			throw std::runtime_error("Could not present Vulkan swapchain image, VulkanContext::PresentSwapchain failed. Encountered VulkanContext::PresentSwapchainUnknownException.");
		}
	}

	void UpdateScene(std::shared_ptr<Scene> scene)
	{
		for (auto mesh : scene->GetMeshes())
		{
			context_->UpdateMesh(currentFrame_, mesh, scene->GetTexture(mesh->GetTextureIndex()));
		}

		for (auto particleEffect : scene->GetParticleEffects())
		{
			context_->UpdateParticleEffect(currentFrame_, particleEffect, scene->GetTexture(particleEffect->GetTextureIndex()));
		}
	}

	void RenderSceneObjects(std::shared_ptr<Scene> scene)
	{
		for (auto mesh : scene->GetMeshes())
		{
			RenderMesh(mesh, scene->GetMVP(mesh));
		}

		for (auto particleEffect : scene->GetParticleEffects())
		{
			RenderParticleEffect(particleEffect, scene->GetMVP(particleEffect));
		}
	}

	void UpdateSceneMeshes(std::shared_ptr<Scene> scene, const float deltaTime)
	{
		for (auto mesh : scene->GetMeshes())
		{
			mesh->Update(deltaTime);
		}

		for (auto particleEffect : scene->GetParticleEffects())
		{
			particleEffect->Update(deltaTime);
		}
	}

	void EndFrame()
	{
		currentFrame_ = (currentFrame_ + 1) % maxFramesInFlight_;
	}

	// TODO: [zpuls 2020-08-08T17:40] Should I allow dynamic pipeline binding per mesh/shader/material/etc?
	void RenderMesh(std::shared_ptr<Mesh> mesh, std::array<glm::mat4, 3> mvp)
	{
		auto& commandBuffer = context_->GetCommandBuffer(currentFrame_);
		mesh->BindMeshData(currentFrame_, commandBuffer, mvp);
		context_->GetDescriptorSet(mesh->GetDescriptorSetIndex())->Bind(currentFrame_, context_->GetGraphicsPipelineLayout(), commandBuffer);
		mesh->Draw(commandBuffer);
	}

	void RenderParticleEffect(std::shared_ptr<ParticleEffect> particleEffect, std::array<glm::mat4, 3> mvp)
	{
		auto& commandBuffer = context_->GetCommandBuffer(currentFrame_);
		particleEffect->BindMeshData(currentFrame_, commandBuffer, mvp);
		context_->GetDescriptorSet(particleEffect->GetDescriptorSetIndex())->Bind(currentFrame_, context_->GetGraphicsPipelineLayout(), commandBuffer);
		particleEffect->Draw(commandBuffer);
	}

private:
	std::shared_ptr<VulkanContext> context_;
	const uint32_t maxFramesInFlight_;
	size_t currentFrame_ = 0;
};
