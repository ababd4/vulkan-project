#include "vk_renderer.h"

#include "../Util/Util.h"
#include <array>

void Renderer::Init(VulkanContext* context, Window& window)
{
	m_context = context;
	m_buffer.Init(*context);
	m_swapchain.init(m_context, window.GetWindowExtent().width, window.GetWindowExtent().height);

	CreateCommandPool();
	CreateCommandBuffers();
	CreateDescriptorAllocator();
	CreateRenderPass();
	CreateFramebuffer();
	CreatePipeline();
}

void Renderer::Cleanup()
{
	m_buffer.Cleanup();

	for (size_t i = 0; i < m_framebuffer.size(); i++) {
		vkDestroyFramebuffer(m_context->GetDevice(), m_framebuffer[i], nullptr);
	}

	m_swapchain.cleanup();
	m_descriptorAllocator.clear(m_context->GetDevice());
	vkDestroyCommandPool(m_context->GetDevice(), m_commandPool, nullptr);
	vkDestroyDescriptorSetLayout(m_context->GetDevice(), m_drawImageDescriptorLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_context->GetDevice(), m_layout, nullptr);
	vkDestroyPipeline(m_context->GetDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_context->GetDevice(), m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_context->GetDevice(), m_renderPass, nullptr);
}

void Renderer::Render()
{

}

void Renderer::CreateCommandPool()
{
	VkCommandPoolCreateInfo command_pool_create_info{};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.pNext = nullptr;
	command_pool_create_info.flags = 0;
	command_pool_create_info.queueFamilyIndex = m_context->GetQueueFamilyIndex();

	VK_CHECK( vkCreateCommandPool(m_context->GetDevice(), &command_pool_create_info, nullptr, &m_commandPool) );
}

void Renderer::CreateCommandBuffers()
{
	m_commandBuffers.resize(MAX_FRAME);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

	VK_CHECK( vkAllocateCommandBuffers(m_context->GetDevice(), &allocInfo, m_commandBuffers.data()) );
}

void Renderer::CreateDescriptorAllocator()
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

void Renderer::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_swapchain.GetSwapchainImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VK_CHECK( vkCreateRenderPass(m_context->GetDevice(), &renderPassInfo, nullptr, &m_renderPass) );
}

void Renderer::CreateFramebuffer()
{
	m_framebuffer.resize(m_swapchain.GetSwapchainImageViews().size());

	for (size_t i = 0; i < m_swapchain.GetSwapchainImageViews().size(); i++) {
		std::array<VkImageView, 1> attachments = {
				m_swapchain.GetSwapchainImageViews()[i]
				// Feature: add depth imageview
		};

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.width = m_swapchain.GetSwapchainExtent().width;
		framebufferCreateInfo.height = m_swapchain.GetSwapchainExtent().height;
		framebufferCreateInfo.layers = 1;
		framebufferCreateInfo.renderPass = m_renderPass;
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferCreateInfo.pAttachments = attachments.data();

		VK_CHECK(vkCreateFramebuffer(m_context->GetDevice(), &framebufferCreateInfo, nullptr, &m_framebuffer[i]));
	}
}

void Renderer::CreatePipeline()
{
	VkShaderModule vertexShader;
	if (!vkutil::LoadShaderModule("res/shaders/mesh.vert.spv", m_context->GetDevice(), &vertexShader)) {
		fmt::print("Error when building the vertex shader \n");
	}

	VkShaderModule fragmentShader;
	if (!vkutil::LoadShaderModule("res/shaders/mesh.frag.spv", m_context->GetDevice(), &fragmentShader)) {
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
	m_pipeline = pipelineBuilder.BuildPipeline(m_context->GetDevice());

	vkDestroyShaderModule(m_context->GetDevice(), fragmentShader, nullptr);
	vkDestroyShaderModule(m_context->GetDevice(), vertexShader, nullptr);
}