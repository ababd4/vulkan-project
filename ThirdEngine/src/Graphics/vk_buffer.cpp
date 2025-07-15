#include "vk_Buffer.h"

#include "../Util/Util.h"

void Buffer::Init(VulkanContext& context)
{
	temp_allocator = context.GetAllocator();
	CreateStagingBuffer();
}

void Buffer::CreateStagingBuffer()
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
		&m_staging_buffer, 
		&m_staging_buffer_allocation, 
		nullptr) 
	);
}

void Buffer::Cleanup()
{
	vmaDestroyBuffer(temp_allocator, m_staging_buffer, m_staging_buffer_allocation);
}