#pragma once

#include "vk_Descriptors.h"
#include "../../Graphics/vk_context.h"

class DescriptorSetManager {
public:
	void init(VulkanContext& context);
	void cleanup();

	VkDescriptorSet getGpuSceneDataDescriptorSet() const { return _gpuSceneDataDescriptorSet; }

private:
	VulkanContext* _context;
	DescriptorAllocatorGrowable _allocator;

	VkDescriptorSet _gpuSceneDataDescriptorSet;
};

