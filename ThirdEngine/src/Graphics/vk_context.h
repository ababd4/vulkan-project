#pragma once

#include "../Util/types.h"
#include "../Window/Window.h"
#include "VkBootstrap/VkBootstrap.h"

constexpr bool bUseValidationLayers = true;

class VulkanContext
{
public:

	void init(Window* window);
	void cleanup();

	VkInstance GetInstance() { return m_instance; };
	VkDevice GetDevice() { return m_device; };
	VkPhysicalDevice GetPhysicalDevice() { return m_physicalDevice; };
	VkSurfaceKHR GetSurface() { return m_surface; };
	VkQueue GetGraphicsQueue() { return m_graphicsQueue; };
	VkQueue GetPresentQueue() { return m_presentQueue; };
	VmaAllocator GetAllocator() { return m_allocator; };
	uint32_t GetQueueFamilyIndex() { return m_queue_family_index; };

private:
	// vulkan
	VkInstance m_instance = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
	vkb::Instance vkb_inst;

	// queue
	uint32_t m_queue_family_index = 0;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue m_presentQueue = VK_NULL_HANDLE;

	// memory allocator
	VmaAllocator m_allocator;

	void create_context(Window* window);
	void create_allocator();
};

