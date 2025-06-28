#include "vk_buffer.h"

#include "../Util/util.h"

void Buffer::init(VulkanContext& context)
{
	temp_allocator = context.GetAllocator();
	create_staging_buffer();
}

void Buffer::create_staging_buffer()
{
	VmaAllocationCreateInfo staging_buffer_allocate_info{};

	staging_buffer_allocate_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = nullptr;
	buffer_create_info.size = 1024;
	buffer_create_info.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT | 
								VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buffer_create_info.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = nullptr;

	VK_CHECK( vmaCreateBuffer(
		temp_allocator, 
		&buffer_create_info, 
		&staging_buffer_allocate_info, 
		&_staging_buffer, 
		&_staging_buffer_allocation, 
		nullptr) 
	);
}

void Buffer::cleanup()
{
	vmaDestroyBuffer(temp_allocator, _staging_buffer, _staging_buffer_allocation);
}