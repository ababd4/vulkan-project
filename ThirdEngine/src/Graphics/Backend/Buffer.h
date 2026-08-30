#pragma once

#include "Core/Types.h"

namespace Buffer
{
	AllocatedBuffer CreateBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, bool debug, const char* name);
	void DestroyBuffer(VmaAllocator allocator, AllocatedBuffer buffer, bool debug, const char* name);
	AllocatedBuffer CreateStagingBuffer(VmaAllocator allocator, size_t);
}