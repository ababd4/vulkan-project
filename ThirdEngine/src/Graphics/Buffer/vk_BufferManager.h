#pragma once

#include "../../Graphics/vk_Context.h"
#include "../vk_Types.h"

#include <functional>

class BufferManager
{
public:

	void Init(VulkanContext* context);
	void Cleanup();

	AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	AllocatedBuffer CreateStagingBuffer(size_t);

	void DestroyBuffer(AllocatedBuffer buffer);

	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

private:
	VulkanContext* m_pContext;

	// Immediate submit structures
	VkFence m_immFence;
	VkCommandBuffer m_immCommandBuffer;
	VkCommandPool m_immCommandPool;

	void CreateSubmitStructures();
};

