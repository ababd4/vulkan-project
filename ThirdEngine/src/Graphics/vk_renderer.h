#pragma once

#include "../Graphics/vk_Context.h"
#include "../Graphics/Pipeline/vk_PipelineManager.h"
#include "../Graphics/Asset/AssetManager.h"
#include "../Graphics/Buffer/Buffer.h"
#include "../Graphics/vk_Descriptors.h"
#include "../Graphics/vk_Swapchain.h"
#include "../Graphics/vk_Init.h"
#include "../Graphics/Scene/vk_Scene.h"
#include "../Window/Window.h"
#include "vk_Types.h"

#include <../../vendor/include/SDL/SDL.h>

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
		PipelineManager* pipelineManager, 
		AssetManager* AssetManager, 
		Scene* scene
	);

	void Cleanup();
	void Render();
	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

private:
	VulkanContext* m_pContext;
	PipelineManager* m_pPipelineManager;
	AssetManager* m_pAssetManager;
	Scene* m_pScene;

	// Push Constants
	GPUDrawPushConstants m_pushConstants;

	// Descriptor
	DescriptorAllocatorGrowable m_descriptorAllocator;
	VkDescriptorSetLayout m_layout;

	// Drawing image
	VkDescriptorSet m_drawImageDescriptors;
	VkDescriptorSetLayout m_drawImageDescriptorLayout;

	// Pipeline Description
	std::vector<PipelineDesc> m_pipelineDesc;

	// Swapchain
	Swapchain m_swapchain;
		
	// Render Pass
	VkRenderPass m_renderPass;
	uint32_t subpass;

	// Immediate SyncObjects to copy CPU Memory to GPU
	VkFence m_immFence;
	VkCommandBuffer m_immCommandBuffer;
	VkCommandPool m_immCommandPool;

	// Frame
	std::vector<VkFramebuffer> m_framebuffers;
	uint32_t currentFrameIndex = 0;
	FrameResource m_frameResources[MAX_FRAME];
	FrameResource& GetCurrentFrame() { return m_frameResources[currentFrameIndex % MAX_FRAME]; }

	// Scene Data
	GPUSceneData m_sceneData;
	VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout;

	// Deletion Queue
	DeletionQueue m_deletionQueue;

	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateRenderPass();
	void CreateFramebuffer();
	void CreateDescriptorAllocator();
	void CreatePipeline();
	void CreateSyncObjects();
	void CreateSubmitStructures();

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void RecreateSwapchain();
};

