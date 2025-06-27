#pragma once

#include "../Graphics/vk_context.h"
#include "../Util/types.h"

class Buffer
{
public:

	void init(VulkanContext& context);
	void cleanup();

	VkBuffer getStagingBuffer() { return _staging_buffer; };

private:

	void create_staging_buffer();
	VkBuffer _staging_buffer;
	VmaAllocator temp_allocator;
	VmaAllocation _staging_buffer_allocation;

};

