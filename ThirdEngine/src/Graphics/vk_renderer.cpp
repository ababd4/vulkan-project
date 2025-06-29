#include "vk_renderer.h"

#include "../Util/util.h"

void Renderer::init(VulkanContext* context, Window& window)
{
	m_context = context;
	m_buffer.init(*context);
	m_swapchain.init(m_context, window.GetWindowExtent().width, window.GetWindowExtent().height);

	create_command_pool();
	create_descriptor_allcator();
	create_pipeline();
}

void Renderer::cleanup()
{
	m_buffer.cleanup();
	m_swapchain.cleanup();
	m_descriptorAllocator.clear(m_context->GetDevice());
	vkDestroyCommandPool(m_context->GetDevice(), m_commandPool, nullptr);
	vkDestroyDescriptorSetLayout(m_context->GetDevice(), m_drawImageDescriptorLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_context->GetDevice(), m_layout, nullptr);
	vkDestroyPipeline(m_context->GetDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_context->GetDevice(), m_pipelineLayout, nullptr);
}

void Renderer::create_command_pool()
{
	VkCommandPoolCreateInfo command_pool_create_info{};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.pNext = nullptr;
	command_pool_create_info.flags = 0;
	command_pool_create_info.queueFamilyIndex = m_context->GetQueueFamilyIndex();

	VK_CHECK( vkCreateCommandPool(m_context->GetDevice(), &command_pool_create_info, nullptr, &m_commandPool) );
}

void Renderer::create_descriptor_allcator()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
	};

	m_descriptorAllocator.init(m_context->GetDevice(), 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		std::cout << "_drawImageDescriptorLayout" << std::endl;
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		m_drawImageDescriptorLayout = builder.build(m_context->GetDevice(), VK_SHADER_STAGE_COMPUTE_BIT);
	}

	m_drawImageDescriptors = m_descriptorAllocator.allocate(m_context->GetDevice(), m_drawImageDescriptorLayout);

	{
		DescriptorWriter writer;
		writer.write_image(0, m_swapchain.GetDrawImage().imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		writer.update_set(m_context->GetDevice(), m_drawImageDescriptors);
	}
}

void Renderer::create_pipeline()
{
	VkShaderModule vertexShader;
	if (!vkutil::load_shader_module("res/shaders/mesh.vert.spv", m_context->GetDevice(), &vertexShader)) {
		fmt::print("Error when building the vertex shader \n");
	}

	VkShaderModule fragmentShader;
	if (!vkutil::load_shader_module("res/shaders/mesh.frag.spv", m_context->GetDevice(), &fragmentShader)) {
		fmt::print("Error when building the vertex shader \n");
	}

	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	m_layout = layoutBuilder.build(m_context->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayout layouts[] = { m_layout };

	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
	pipeline_layout_info.pPushConstantRanges = &bufferRange;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pSetLayouts = layouts;
	pipeline_layout_info.setLayoutCount = 1;
	VK_CHECK( vkCreatePipelineLayout(m_context->GetDevice(), &pipeline_layout_info, nullptr, &m_pipelineLayout) );

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(vertexShader, fragmentShader);
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.disable_blending();
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_LESS);

	pipelineBuilder._pipelineLayout = m_pipelineLayout;
	m_pipeline = pipelineBuilder.build_pipeline(m_context->GetDevice());

	vkDestroyShaderModule(m_context->GetDevice(), fragmentShader, nullptr);
	vkDestroyShaderModule(m_context->GetDevice(), vertexShader, nullptr);
}