#pragma once

#include "vk_Types.h"

namespace vkinit {
	VkPipelineShaderStageCreateInfo CreatePipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry = "main");
	VkPipelineLayoutCreateInfo CreatePipelineLayoutCreateInfo();
	VkImageCreateInfo CreateImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	VkImageViewCreateInfo CreateImageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
	VkCommandBufferBeginInfo CreateCommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
	VkRenderPassBeginInfo CreateRenderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer frameBuffer, int32_t offset_x, int32_t offset_y, VkExtent2D extent, const VkClearValue* clear);
	VkCommandBufferSubmitInfo CreateCommandBufferSubmitInfo(VkCommandBuffer commandBuffer);
	VkSemaphoreSubmitInfo CreateSemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
	VkSubmitInfo2 CreateSubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);
	VkPresentInfoKHR CreatePresentInfo();
}

