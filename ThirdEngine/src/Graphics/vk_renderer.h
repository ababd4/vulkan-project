#pragma once

#include "../Graphics/vk_Context.h"
#include "../Graphics/Pipeline/vk_PipelineManager.h"
#include "../Graphics/Mesh/vk_MeshManager.h"
#include "../Graphics/vk_Descriptors.h"
#include "../Graphics/vk_Swapchain.h"
#include "../Window/Window.h"
#include "../Graphics/vk_Init.h"
#include "vk_Types.h"

struct FrameResource {
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	VkSemaphore swapchainSemaphore, renderSemaphore;
	VkFence renderFence;
};

constexpr int MAX_FRAME = 2;

class Renderer
{
public:

	void Init(VulkanContext* context, Window& window, PipelineManager* pipelineManager, MeshManager* meshManager);
	void Cleanup();

	void UpdateScene();
	void Render();

private:
	VulkanContext* m_pContext;
	PipelineManager* m_pPipelineManager;
	MeshManager* m_pMeshManager;

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

	// Frame
	std::vector<VkFramebuffer> m_framebuffers;
	uint32_t currentFrameIndex = 0;
	FrameResource m_frameResources[MAX_FRAME];
	FrameResource& GetCurrentFrame() { return m_frameResources[currentFrameIndex % MAX_FRAME]; }

	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateRenderPass();
	void CreateFramebuffer();
	void CreateDescriptorAllocator();
	void CreatePipeline();
	void CreateSyncObjects();

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void RecreateSwapchain();
};

