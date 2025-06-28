#pragma once

#include "../Graphics/vk_context.h"
#include "../Util/types.h"

class Swapchain {
public:
	void init(VulkanContext* context, uint32_t width, uint32_t height);
	void cleanup();

	VkSwapchainKHR GetSwapchain() { return m_swapchain; }
	VkFormat GetSwapchainImageFormat() { return m_swapchainImageFormat; }
	std::vector<VkImage> GetSwapchainImages() { return m_swapchainImages; }
	std::vector<VkImageView> GetSwapchainImageViews() { return m_swapchainImageViews; }
	VkExtent2D GetSwapchainExtent() { return m_swapchainExtent; }
	AllocatedImage GetDrawImage() { return m_drawImage; }
	AllocatedImage GetDepthImage() { return m_depthImage; }

private:
	VulkanContext* m_context;

	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchainImageFormat;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	VkExtent2D m_swapchainExtent;

	AllocatedImage m_drawImage;
	AllocatedImage m_depthImage;

	void create_swapchain(uint32_t width, uint32_t height);
};