#pragma once

#include "../Graphics/vk_context.h"
#include "../Graphics/vk_buffer.h"
#include "../Graphics/vk_pipeline.h"
#include "../Util/types.h"

class Renderer
{
public:

	void init(VulkanContext& context);
	void cleanup();

private:
	VulkanContext* _context;
	
	// command pool
	VkCommandPool _commandPool;

	// buffer
	Buffer _buffer;

	// pipeline
	VkPipeline _pipeline;

	void create_command_pool();
	void create_pipeline();
};

