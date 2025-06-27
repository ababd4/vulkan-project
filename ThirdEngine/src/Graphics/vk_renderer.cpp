#include "vk_renderer.h"

#include "../Util/util.h"

void Renderer::init(VulkanContext& context)
{
	_context = &context;
	_buffer.init(context);
	

	create_command_pool();
	create_descriptor_allcator();
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

void Renderer::create_descriptor_allcator()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
	};

	_descriptorAllocator.init(_context->getDevice(), 10, sizes);


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

	DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	_layout = layoutBuilder.build(_context->getDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayout layouts[] = { _layout };

	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
	pipeline_layout_info.pPushConstantRanges = &bufferRange;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pSetLayouts = layouts;
	pipeline_layout_info.setLayoutCount = 1;
	VK_CHECK( vkCreatePipelineLayout(_context->getDevice(), &pipeline_layout_info, nullptr, &_pipelineLayout) );

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(vertexShader, fragmentShader);
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.disable_blending();
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_LESS);

	pipelineBuilder._pipelineLayout = _pipelineLayout;
	_pipeline = pipelineBuilder.build_pipeline(_context->getDevice());

	vkDestroyShaderModule(_context->getDevice(), fragmentShader, nullptr);
	vkDestroyShaderModule(_context->getDevice(), vertexShader, nullptr);
}