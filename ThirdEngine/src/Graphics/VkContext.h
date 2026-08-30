#pragma once

#include "Types.h"
#include "../Window/Window.h"
#include "VkBootstrap/VkBootstrap.h"

#ifdef _DEBUG
constexpr bool bUseValidationLayers = true;
#else
constexpr bool bUseValidationLayers = false;
#endif

class VulkanContext
{
public:

	void Init(Window* window);
	void Cleanup();
	void PrintAllocationCount(std::string str);

	VkInstance GetInstance() { return m_instance; };
	VkDevice GetDevice() { return m_device; };
	VkPhysicalDevice GetPhysicalDevice() { return m_physicalDevice; };
	VkSurfaceKHR GetSurface() { return m_surface; };
	VkQueue GetGraphicsQueue() { return m_graphicsQueue; };
	VkQueue GetPresentQueue() { return m_presentQueue; };
	VkQueue GetTransferQueue() { return m_transferQueue; };
	VmaAllocator& GetAllocator() { return m_allocator; };
	uint32_t GetGraphicsQueueFamilyIndex() { return m_graphicsQueueFamilyIndex; };
	uint32_t GetTransferQueueFamilyIndex() { return m_transferQueueFamilyIndex; };

private:
	// vulkan
	VkInstance m_instance = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
	vkb::Instance vkb_inst;

	// queue
	uint32_t m_graphicsQueueFamilyIndex = 0;
	uint32_t m_transferQueueFamilyIndex = 0;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue m_presentQueue = VK_NULL_HANDLE;
	VkQueue m_transferQueue = VK_NULL_HANDLE;

	// memory allocator
	VmaAllocator m_allocator;

	void CreateContext(Window* window);
	void CreateAllocator();
};

