#pragma once

#include "vk_PipelineBuilder.h"
#include "../vk_Types.h"
#include "../vk_Context.h"
#include <unordered_map>
#include <string>

class PipelineManager {
public:
	void Init(VulkanContext* context);
	void Cleanup();
	VkPipeline GetPipeline(PipelineDesc desc);
	VkPipelineLayout GetPipelineLayout() { return m_pipelineLayout; };

private:
	VulkanContext* m_pContext;

	std::unordered_map<PipelineDesc, VkPipeline, PipelineDescHash> m_pipelines;
	PipelineBuilder m_PipelineBuilder;

	VkPipelineLayout m_pipelineLayout;

	MaterialPipeline opaquePipeline;
	MaterialPipeline transparentPipeline;

	VkPipeline BuildPipeline(PipelineDesc desc);
};

