#pragma once

#include "../Graphics/vk_Context.h"
#include "../Graphics/vk_Buffer.h"
#include "../Graphics/vk_Pipeline.h"
#include "../Graphics/vk_Descriptors.h"
#include "../Graphics/vk_Swapchain.h"
#include "../Window/Window.h"
#include "../Util/Types.h"

constexpr int MAX_FRAME = 2;

class Renderer
{
public:

	void Init(VulkanContext* context, Window& window);
	void Cleanup();

	void Render();

private:
	VulkanContext* m_context;
	
	// Command
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

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

	// Frame Buffer
	std::vector<VkFramebuffer> m_framebuffer;

	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateRenderPass();
	void CreateFramebuffer();
	void CreatePipeline();
	void CreateDescriptorAllocator();
};

