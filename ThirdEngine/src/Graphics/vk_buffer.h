#pragma once

#include "../Graphics/vk_Context.h"
#include "../Util/Types.h"

class Buffer
{
public:

	void Init(VulkanContext& context);
	void Cleanup();

	VkBuffer GetStagingBuffer() { return m_staging_buffer; };

private:

	void CreateStagingBuffer();
	VkBuffer m_staging_buffer;
	VmaAllocator temp_allocator;
	VmaAllocation m_staging_buffer_allocation;

};

