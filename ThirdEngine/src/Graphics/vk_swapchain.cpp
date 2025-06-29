#include "vk_swapchain.h"

#include "VkBootstrap/VkBootstrap.h"
#include "../Graphics/vk_init.h"
#include "../Util/util.h"

void Swapchain::init(VulkanContext* context, uint32_t width, uint32_t height)
{
	m_context = context;
	
	create_swapchain(width, height);

	//draw image size will match the window
	VkExtent3D drawImageExtent = {
		width,
		height,
		1
	};

	//hardcoding the draw format to 32 bit float
	m_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	m_drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info = vkinit::image_create_info(m_drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(m_context->GetAllocator(), &rimg_info, &rimg_allocinfo, &m_drawImage.image, &m_drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(m_drawImage.imageFormat, m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(m_context->GetDevice(), &rview_info, nullptr, &m_drawImage.imageView));

	m_depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
	m_depthImage.imageExtent = drawImageExtent;
	VkImageUsageFlags depthImageUsages{};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkImageCreateInfo dimg_info = vkinit::image_create_info(m_depthImage.imageFormat, depthImageUsages, drawImageExtent);

	//allocate and create the image
	vmaCreateImage(m_context->GetAllocator(), &dimg_info, &rimg_allocinfo, &m_depthImage.image, &m_depthImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(m_depthImage.imageFormat, m_depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(m_context->GetDevice(), &dview_info, nullptr, &m_depthImage.imageView));
}

void Swapchain::cleanup()
{
	vkDestroySwapchainKHR(m_context->GetDevice(), m_swapchain, nullptr);

	vkDestroyImageView(m_context->GetDevice(), m_drawImage.imageView, nullptr);
	vmaDestroyImage(m_context->GetAllocator(), m_drawImage.image, m_drawImage.allocation);

	vkDestroyImageView(m_context->GetDevice(), m_depthImage.imageView, nullptr);
	vmaDestroyImage(m_context->GetAllocator(), m_depthImage.image, m_depthImage.allocation);

	for (int i = 0; i < m_swapchainImageViews.size(); i++) {
		vkDestroyImageView(m_context->GetDevice(), m_swapchainImageViews[i], nullptr);
	}
}

void Swapchain::create_swapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchainBuilder{ m_context->GetPhysicalDevice(), m_context->GetDevice(), m_context->GetSurface() };

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
	m_swapchainImageViews = vkbSwapchain.get_image_views().value();
}