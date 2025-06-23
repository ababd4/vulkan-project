#pragma once

#include "../Util/types.h"

class VulkanContext
{
public:

	void init();
	void cleanup();

	VkInstance getInstance() { return _instance; };
	VkDevice getDevice() { return _device; };
	VkPhysicalDevice getPhysicalDevice() { return _physicalDevice; };
	VkSurfaceKHR getSurface() { return _surface; };
	VkQueue getGraphicsQueue() { return _graphicsQueue; };
	VkQueue getPresentQueue() { return _presentQueue; };
	VmaAllocator getAllocator() { return _allocator; };
	uint32_t getQueueFamilyIndex() { return _queue_family_index; };

private:
	// vulkan
	VkInstance _instance = VK_NULL_HANDLE;
	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;

	// queue
	uint32_t _queue_family_index = 0;
	VkQueue _graphicsQueue = VK_NULL_HANDLE;
	VkQueue _presentQueue = VK_NULL_HANDLE;

	// memory allocator
	VmaAllocator _allocator;

	void create_instance();
	void create_device();
	void create_allocator();

};

