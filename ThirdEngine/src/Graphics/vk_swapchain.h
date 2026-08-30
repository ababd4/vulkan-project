#pragma once

#include "../Graphics/vk_Context.h"
#include "vk_Types.h"

class Swapchain {
public:
	void Init(VulkanContext* context, uint32_t width, uint32_t height);
	void Cleanup();

	VkSwapchainKHR GetSwapchain() { return m_swapchain; }
	VkFormat GetSwapchainImageFormat() { return m_swapchainImageFormat; }
	std::vector<VkImage> GetSwapchainImages() { return m_swapchainImages; }
	std::vector<VkImageView> GetSwapchainImageViews() { return m_swapchainColorImageViews; } // for imgui
	VkExtent2D GetSwapchainExtent() { return m_swapchainExtent; }
	VkSemaphore GetPresentSemaphoreByIndex(uint32_t imageIndex) { return m_presentSemaphores[imageIndex]; }

private:
	VulkanContext* m_pContext;

	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchainImageFormat;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainColorImageViews;
	VkExtent2D m_swapchainExtent;

	// semaphore for present image
	std::vector<VkSemaphore> m_presentSemaphores;

	void CreateSwapchain(uint32_t width, uint32_t height);
	void CreatePresentSemaphore();
	void DestroyPresentSemaphore();
};