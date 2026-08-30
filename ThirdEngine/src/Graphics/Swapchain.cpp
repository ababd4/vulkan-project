#include "Swapchain.h"

#include "VkBootstrap/VkBootstrap.h"
#include "../Graphics/Init.h"
#include "../Util/Util.h"

void Swapchain::Init(VulkanContext* context, uint32_t width, uint32_t height)
{
	m_pContext = context;
	
	CreateSwapchain(width, height);
	CreatePresentSemaphore();
}

void Swapchain::Cleanup()
{
	DestroyPresentSemaphore();

	for (int i = 0; i < m_swapchainColorImageViews.size(); i++) {
		vkDestroyImageView(m_pContext->GetDevice(), m_swapchainColorImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(m_pContext->GetDevice(), m_swapchain, nullptr);
}



void Swapchain::CreateSwapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchainBuilder{ m_pContext->GetPhysicalDevice(), m_pContext->GetDevice(), m_pContext->GetSurface() };

	m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = m_swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	m_swapchainExtent = vkbSwapchain.extent;

	// store swapchain and its related images
	m_swapchain = vkbSwapchain.swapchain;
	m_swapchainImages = vkbSwapchain.get_images().value();
	m_swapchainColorImageViews = vkbSwapchain.get_image_views().value();
}

void Swapchain::CreatePresentSemaphore()
{
	DestroyPresentSemaphore();
	const uint32_t imageCount = GetSwapchainImages().size();

	// std::cout << imageCount << std::endl;

	m_presentSemaphores.resize(imageCount);

	VkSemaphoreCreateInfo ci{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	for (uint32_t i = 0; i < imageCount; ++i) {
		VK_CHECK(vkCreateSemaphore(m_pContext->GetDevice(), &ci, nullptr, &m_presentSemaphores[i]));
	}
}

void Swapchain::DestroyPresentSemaphore()
{
	for (auto& semaphore : m_presentSemaphores) {
		if (semaphore) {
			vkDestroySemaphore(m_pContext->GetDevice(), semaphore, nullptr);
			semaphore = VK_NULL_HANDLE;
		}
	}
	m_presentSemaphores.clear();
}