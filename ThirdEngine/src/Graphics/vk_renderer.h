#pragma once

#include "../Graphics/vk_context.h"
#include "../Graphics/vk_buffer.h"
#include "../Graphics/vk_pipeline.h"
#include "../Graphics/Descriptor/vk_DescriptorManager.h"
#include "../Graphics/vk_swapchain.h"
#include "../Window/Window.h"
#include "../Util/types.h"

class Renderer
{
public:

	void init(VulkanContext* context, Window& window);
	void cleanup();

private:
	VulkanContext* m_context;
	
	// Command Pool
	VkCommandPool m_commandPool;

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

	// drawing image
	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;

	// swapchain
	Swapchain m_swapchain;

	void create_command_pool();
	void create_pipeline();
	void create_descriptor_allcator();
};

