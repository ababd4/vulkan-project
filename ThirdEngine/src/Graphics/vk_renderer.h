#pragma once

#include "../Graphics/vk_context.h"
#include "../Graphics/vk_buffer.h"
#include "../Graphics/vk_pipeline.h"
#include "../Graphics/Descriptor/vk_DescriptorManager.h"
#include "../Util/types.h"

class Renderer
{
public:

	void init(VulkanContext& context);
	void cleanup();

private:
	VulkanContext* _context;
	
	// Command Pool
	VkCommandPool _commandPool;

	// Buffer
	Buffer _buffer;

	// Pipeline
	VkPipeline _pipeline;
	VkPipelineLayout _pipelineLayout;

	// Push Constants
	GPUDrawPushConstants _pushConstants;

	// Descriptor
	DescriptorAllocatorGrowable _descriptorAllocator;
	VkDescriptorSetLayout _layout;

	// drawing image
	AllocatedImage _drawImage;
	AllocatedImage _depthImage;

	void create_command_pool();
	void create_pipeline();
	void create_descriptor_allcator();
};

