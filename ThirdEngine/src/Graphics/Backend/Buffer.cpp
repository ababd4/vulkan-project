#include "Buffer.h"

#include "Util/Util.h"

AllocatedBuffer Buffer::CreateBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, bool debug, const char* name)
{
	// create buffer
	VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;

	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocatedBuffer newBuffer;

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));
	vmaSetAllocationName(allocator, newBuffer.allocation, name);

	if (debug) {
		std::cout
			<< "[CREATE BUFFER] "
			<< newBuffer.allocation
			<< " size=" << allocSize
			<< std::endl;
	}

	return newBuffer;
}

void Buffer::DestroyBuffer(VmaAllocator allocator, AllocatedBuffer buffer, bool debug, const char* name) 
{
	if (debug) {
		std::cout
			<< "[DESTROY BUFFER] "
			<< buffer.allocation
			<< std::endl;
	}
	vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

AllocatedBuffer Buffer::CreateStagingBuffer(VmaAllocator allocator, size_t allocSize)
{
	// create buffer
	VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;

	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocatedBuffer newBuffer;

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

	return newBuffer;
}