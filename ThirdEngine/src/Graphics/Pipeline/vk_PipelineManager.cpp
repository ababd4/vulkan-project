#include "vk_PipelineManager.h"

#include "../vk_Descriptors.h"
#include "../../Util/Util.h"

void PipelineManager::Init(VulkanContext* context) 
{
	m_pContext = context;


}

void PipelineManager::Cleanup() 
{
	for (auto& mp : m_materialPipelines) {
		if (mp.second.pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(m_pContext->GetDevice(), mp.second.pipeline, nullptr);
			vkDestroyPipelineLayout(m_pContext->GetDevice(), mp.second.layout, nullptr);
		}
	}
	vkDestroyRenderPass(m_pContext->GetDevice(), m_renderPass, nullptr);
}

void PipelineManager::InitPipelines(Swapchain swapchain) 
{
	

	m_materialPipelines[PipelineType::GltfOpaque] = BuildPipeline("res/shaders/GltfModel.vert.spv", "res/shaders/GltfModel.frag.spv", m_renderPass, 0, PipelineType::GltfOpaque);
	m_materialPipelines[PipelineType::GltfTransparent] = BuildPipeline("res/shaders/GltfModel.vert.spv", "res/shaders/GltfModel.frag.spv", m_renderPass, 0, PipelineType::GltfTransparent);
}