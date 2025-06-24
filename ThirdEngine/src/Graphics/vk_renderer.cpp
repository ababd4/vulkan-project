#include "vk_renderer.h"

#include "../Util/util.h"

void Renderer::init(VulkanContext& context)
{
	_context = &context;
	_buffer.init(context);
	_descriptorSetManager.init(context);

	create_command_pool();
	create_pipeline();
}

void Renderer::cleanup()
{
	_buffer.cleanup();
	vkDestroyCommandPool(_context->getDevice(), _commandPool, nullptr);
}

void Renderer::create_command_pool()
{
	VkCommandPoolCreateInfo command_pool_create_info{};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.pNext = nullptr;
	command_pool_create_info.flags = 0;
	command_pool_create_info.queueFamilyIndex = _context->getQueueFamilyIndex();

	VK_CHECK( vkCreateCommandPool(_context->getDevice(), &command_pool_create_info, nullptr, &_commandPool) );
}

void Renderer::create_pipeline()
{
	VkShaderModule vertexShader;
	if (!vkutil::load_shader_module("res/shaders/mesh.vert.spv", _context->getDevice(), &vertexShader)) {
		fmt::print("Error when building the vertex shader \n");
	}

	VkShaderModule fragmentShader;
	if (!vkutil::load_shader_module("res/shaders/mesh.frag.spv", _context->getDevice(), &fragmentShader)) {
		fmt::print("Error when building the vertex shader \n");
	}

	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
	//pipeline_layout_info.pPushConstantRanges = &bufferRange;
	//pipeline_layout_info.pushConstantRangeCount = 1;
	//pipeline_layout_info.pSetLayouts = &_singleImageDescriptorLayout;
	//pipeline_layout_info.setLayoutCount = 2;
	//VK_CHECK(vkCreatePipelineLayout(_context->getDevice(), &pipeline_layout_info, nullptr, &_pipelineLayout));
}