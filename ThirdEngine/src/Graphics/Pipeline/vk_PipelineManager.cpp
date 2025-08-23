#include "vk_PipelineManager.h"

#include "../vk_Descriptors.h"
#include "../../Util/Util.h"

void PipelineManager::Init(VulkanContext* context) 
{
	m_pContext = context;
}

void PipelineManager::Cleanup() 
{
	for (auto& pipeline : m_pipelines) {
		if (pipeline.second != VK_NULL_HANDLE) {
			vkDestroyPipeline(m_pContext->GetDevice(), pipeline.second, nullptr);
		}
	}
}

VkPipeline PipelineManager::BuildPipeline(PipelineDesc desc)
{
	VkShaderModule vertexShader;
	if (!vkutil::LoadShaderModule(desc.vert, m_pContext->GetDevice(), &vertexShader)) {
		fmt::print("Error when building the vertex shader \n");
	}

	VkShaderModule fragmentShader;
	if (!vkutil::LoadShaderModule(desc.frag, m_pContext->GetDevice(), &fragmentShader)) {
		fmt::print("Error when building the fragment shader \n");
	}

	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	VkDescriptorSetLayout layout = layoutBuilder.build(m_pContext->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayout layouts[] = { layout };

	VkPipelineLayout pipelineLayout;
	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::CreatePipelineLayoutCreateInfo();
	pipeline_layout_info.pPushConstantRanges = &bufferRange;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pSetLayouts = layouts;
	pipeline_layout_info.setLayoutCount = 1;
	VK_CHECK(vkCreatePipelineLayout(m_pContext->GetDevice(), &pipeline_layout_info, nullptr, &pipelineLayout));

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(vertexShader, fragmentShader);
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.disable_blending();
	pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_LESS);

	pipelineBuilder._pipelineLayout = pipelineLayout;
	VkPipeline pipeline = pipelineBuilder.BuildPipeline(m_pContext->GetDevice(), desc.renderPass, desc.subpass);
	// register to pipeline list
	m_pipelines[desc] = pipeline;

	vkDestroyShaderModule(m_pContext->GetDevice(), vertexShader, nullptr);
	vkDestroyShaderModule(m_pContext->GetDevice(), fragmentShader, nullptr);
	vkDestroyPipelineLayout(m_pContext->GetDevice(), pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_pContext->GetDevice(), layout, nullptr);

	return pipeline;
}

VkPipeline PipelineManager::GetPipeline(PipelineDesc desc) {
	// check existing pipeline
	auto it = m_pipelines.find(desc);
	if (it != m_pipelines.end()) {
		return it->second;
	}

	// otherwise create new pipeline
	return BuildPipeline(desc);
}