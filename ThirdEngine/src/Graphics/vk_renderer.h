#pragma once

#include "../Graphics/vk_Context.h"
#include "../Graphics/vk_Buffer.h"
#include "../Graphics/vk_Pipeline.h"
#include "../Graphics/vk_Descriptors.h"
#include "../Graphics/vk_Swapchain.h"
#include "../Window/Window.h"
#include "../Util/Types.h"

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

	void Init(VulkanContext* context, Window& window);
	void Cleanup();

	void UpdateScene();
	void Render();

private:
	VulkanContext* m_context;

	// Buffer
	Buffer m_buffer;

	// Pipeline
	VkPipeline m_pipeline;
	VkPipelineLayout m_pipelineLayout;

	// Push Constants
	GPUDrawPushConstants m_pushConstants;

	// Descriptor
	DescriptorAllocatorGrowable m_descriptorAllocator;
	VkDescriptorSetLayout m_layout;

	// Drawing image
	VkDescriptorSet m_drawImageDescriptors;
	VkDescriptorSetLayout m_drawImageDescriptorLayout;

	// Swapchain
	Swapchain m_swapchain;

	// Render Pass
	VkRenderPass m_renderPass;

	// Frame
	std::vector<VkFramebuffer> m_framebuffers;
	uint32_t currentFrameIndex = 0;
	FrameResource m_frameResources[MAX_FRAME];
	FrameResource& GetCurrentFrame() { return m_frameResources[currentFrameIndex % MAX_FRAME]; }

	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateRenderPass();
	void CreateFramebuffer();
	void CreatePipeline();
	void CreateDescriptorAllocator();
	void CreateSyncObjects();

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};

