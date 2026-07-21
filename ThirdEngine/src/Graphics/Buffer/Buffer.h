#pragma once

#include "../vk_Types.h"

namespace Buffer
{
	AllocatedBuffer CreateBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	AllocatedBuffer CreateStagingBuffer(VmaAllocator allocator, size_t);
}