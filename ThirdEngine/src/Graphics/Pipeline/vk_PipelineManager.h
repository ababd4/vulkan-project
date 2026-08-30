#pragma once

#include "vk_PipelineBuilder.h"
#include "../vk_Types.h"
#include "../vk_Context.h"
#include "../vk_swapchain.h"
#include <unordered_map>
#include <string>

class PipelineManager {
public:
	void Init(VulkanContext* context);
	void Cleanup();
	MaterialPipeline GetPipeline(PipelineType type);

private:
	VulkanContext* m_pContext;

	std::unordered_map<PipelineType, MaterialPipeline> m_materialPipelines;
	PipelineBuilder m_PipelineBuilder;

	MaterialPipeline opaquePipeline;
	MaterialPipeline transparentPipeline;

	VkRenderPass m_renderPass;

	void InitPipelines(Swapchain swapchain);
	MaterialPipeline BuildPipeline(std::string vert, std::string frag, VkRenderPass pass, uint32_t subpass, PipelineType type);
};

