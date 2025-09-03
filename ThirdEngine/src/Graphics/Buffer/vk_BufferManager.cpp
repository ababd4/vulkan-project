#include "vk_BufferManager.h"

#include "../../Util/Util.h"
#include "../vk_Init.h"

void BufferManager::Init(VulkanContext* context)
{
	m_pContext = context;

	CreateSubmitStructures();
}

void BufferManager::Cleanup()
{
	vkDestroyCommandPool(m_pContext->GetDevice(), m_immCommandPool, nullptr);
	vkDestroyFence(m_pContext->GetDevice(), m_immFence, nullptr);
}

void BufferManager::DestroyBuffer(AllocatedBuffer allocatedBuffer)
{
	vmaDestroyBuffer(m_pContext->GetAllocator(), allocatedBuffer.buffer, allocatedBuffer.allocation);
}

AllocatedBuffer BufferManager::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
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
	VK_CHECK(vmaCreateBuffer(m_pContext->GetAllocator(), &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

	return newBuffer;
}

AllocatedBuffer BufferManager::CreateStagingBuffer(size_t allocSize)
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
	VK_CHECK(vmaCreateBuffer(m_pContext->GetAllocator(), &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

	return newBuffer;
}

void BufferManager::CreateSubmitStructures()
{
	VkCommandPoolCreateInfo immCommandPoolCreateInfo{};
	immCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	immCommandPoolCreateInfo.pNext = nullptr;
	immCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	immCommandPoolCreateInfo.queueFamilyIndex = m_pContext->GetGraphicsQueueFamilyIndex();
	VK_CHECK(vkCreateCommandPool(m_pContext->GetDevice(), &immCommandPoolCreateInfo, nullptr, &m_immCommandPool));

	VkCommandBufferAllocateInfo immAllocInfo{};
	immAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	immAllocInfo.commandPool = m_immCommandPool;
	immAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	immAllocInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(m_pContext->GetDevice(), &immAllocInfo, &m_immCommandBuffer));

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VK_CHECK(vkCreateFence(m_pContext->GetDevice(), &fenceInfo, nullptr, &m_immFence));
}

void BufferManager::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(m_pContext->GetDevice(), 1, &m_immFence));
	VK_CHECK(vkResetCommandBuffer(m_immCommandBuffer, 0));

	VkCommandBuffer cmd = m_immCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::CreateCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = vkinit::CreateCommandBufferSubmitInfo(cmd);
	VkSubmitInfo2 submit = vkinit::CreateSubmitInfo(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  m_immFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(m_pContext->GetGraphicsQueue(), 1, &submit, m_immFence));

	VK_CHECK(vkWaitForFences(m_pContext->GetDevice(), 1, &m_immFence, true, 9999999999));
}