#pragma once

#include "../Graphics/VkContext.h"
#include "../Graphics/Asset/AssetManager.h"
#include "../Graphics/Buffer/Buffer.h"
#include "../Graphics/Descriptors.h"
#include "../Graphics/Swapchain.h"
#include "../Graphics/Init.h"
#include "../Graphics/Scene/Scene.h"
#include "../Graphics/UI/ImGuiLayer.h"
#include "../Window/Window.h"
#include "Types.h"

struct FrameResource {
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	VkSemaphore swapchainSemaphore, renderSemaphore;
	VkFence renderFence;

	DeletionQueue deletionQueue;
	DescriptorAllocatorGrowable frameDescriptor;
};

constexpr int MAX_FRAME = 2;

class Renderer
{
public:

	void Init(
		VulkanContext* context,
		Window& window,
		AssetManager* AssetManager,
		Scene* scene,
		ImGuiLayer* imGuiLayer,
		EngineStats* stats
	);

	void Cleanup();
	void Render(ImGuiLayer& imguiLayer);
	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	VkDescriptorSetLayout GetGlobalDescriptorSetLayout() { return m_gpuSceneDataDescriptorLayout; }
	VkDescriptorSetLayout GetShadowDescriptorSetLayout() { return m_shadowResources.descriptorSetLayout; }
	AllocatedImage GetDrawImage() { return m_drawImage; }
	AllocatedImage GetDepthImage() { return m_depthImage; }

	// Temporary define <- should not be here
	// Swapchain
	Swapchain m_swapchain;

private:
	VulkanContext* m_pContext;
	AssetManager* m_pAssetManager;
	Scene* m_pScene;
	ImGuiLayer* m_pImGuiLayer;
	EngineStats* m_pStats;

	// Push Constants
	GPUDrawPushConstants m_pushConstants;

	// Descriptor
	DescriptorAllocatorGrowable m_descriptorAllocator;
	VkDescriptorSetLayout m_layout;

	// Drawing image
	VkDescriptorSet m_drawImageDescriptors;
	VkDescriptorSetLayout m_drawImageDescriptorLayout;
		
	// Render Pass
	//VkRenderPass m_renderPass;
	//uint32_t subpass;

	// Immediate SyncObjects to copy CPU Memory to GPU
	VkFence m_immFence;
	VkCommandBuffer m_immCommandBuffer;
	VkCommandPool m_immCommandPool;

	// Frame
	//std::vector<VkFramebuffer> m_framebuffers;
	uint32_t currentFrameIndex = 0;
	FrameResource m_frameResources[MAX_FRAME];
	FrameResource& GetCurrentFrame() { return m_frameResources[currentFrameIndex % MAX_FRAME]; }

	// Scene Data
	GPUSceneData m_sceneData;
	VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout;

	// Deletion Queue
	DeletionQueue m_deletionQueue;

	// Images
	AllocatedImage m_drawImage;
	AllocatedImage m_depthImage;
	VkExtent2D m_drawExtent;
	float renderScale = 1.f;

	// resources for shadow mapping
	DirectionalShadowResources m_shadowResources;

	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateImages(uint32_t width, uint32_t height);
	//void CreateRenderPass();
	//void CreateFramebuffer();
	void CreateDescriptorAllocator();
	void CreateSyncObjects();
	void CreateSubmitStructures();

	void DrawShadowPass(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);
	void DrawMainPass(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkDescriptorSet globalDescriptor);
	void RecreateSwapchain();
};

