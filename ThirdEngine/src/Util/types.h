#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>

struct GPUDrawPushConstants {
	// push constants
	glm::mat4 worldMatrix;
	VkDeviceAddress vertexBuffer;
};

struct AllocatedImage {
	VkImage image;
	VkImageView imageView;
	VkFormat imageFormat;
	VkExtent3D imageExtent;
	VmaAllocation allocation;
};