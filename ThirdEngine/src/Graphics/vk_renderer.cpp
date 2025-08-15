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
	CreateSyncObjects();
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

void Renderer::UpdateScene()
{

}

void Renderer::Render()
{
	VK_CHECK(vkWaitForFences(m_context->GetDevice(), 1, &m_fences[currentFrameIndex], VK_TRUE, UINT64_MAX));

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_context->GetDevice(), m_swapchain.GetSwapchain(), UINT64_MAX, m_imageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &imageIndex);

	// TODO: resize swapchain
	//if (result == VK_ERROR_OUT_OF_DATE_KHR) {
	//	recreateSwapChain();
	//	return;
	//}
	//else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
	//	throw std::runtime_error("failed to acquire swap chain image!");
	//}

	// reset command buffer
	VK_CHECK(vkResetFences(m_context->GetDevice(), 1, &m_fences[currentFrameIndex]));

	// record image index to command buffer
	RecordCommandBuffer(m_commandBuffers[currentFrameIndex], imageIndex);

	// create command buffer submit info
	VkCommandBufferSubmitInfo cmdSubmitInfo = vkinit::CreateCommandBufferSubmitInfo(m_commandBuffers[currentFrameIndex]);
	VkSemaphoreSubmitInfo waitInfo = vkinit::CreateSemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, m_imageAvailableSemaphores[currentFrameIndex]);
	VkSemaphoreSubmitInfo signalInfo = vkinit::CreateSemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, m_renderFinishedSemaphores[currentFrameIndex]);

	VkSubmitInfo2 submit = vkinit::CreateSubmitInfo(&cmdSubmitInfo, &signalInfo, &waitInfo);

	// submit command buffer to the queue and execute it
	VK_CHECK(vkQueueSubmit2(m_context->GetGraphicsQueue(), 1, &submit, m_fences[currentFrameIndex]));

	// prepare present
	VkPresentInfoKHR presentInfo = vkinit::CreatePresentInfo();

	VkSwapchainKHR swapchains[] = { m_swapchain.GetSwapchain() };
	presentInfo.pSwapchains = swapchains;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[currentFrameIndex];
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &m_swapchain.GetSwapchainImages()[currentFrameIndex];
}

void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo = vkinit::CreateCommandBufferBeginInfo();

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	VkRenderPassBeginInfo renderPassInfo = vkinit::CreateRenderPassBeginInfo(m_renderPass, m_framebuffer[currentFrameIndex], 0, 0, m_swapchain.GetSwapchainExtent());

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind the graphics pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	// Set viewport and sicissor values
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchain.GetSwapchainExtent().width);
	viewport.height = static_cast<float>(m_swapchain.GetSwapchainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapchain.GetSwapchainExtent();
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// TODO: submit vertex,index buffer

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	// render pass can be ended
	vkCmdEndRenderPass(commandBuffer);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));
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

	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::CreatePipelineLayoutCreateInfo();
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

void Renderer::CreateSyncObjects()
{
	m_imageAvailableSemaphores.resize(MAX_FRAME);
	m_renderFinishedSemaphores.resize(MAX_FRAME);
	m_fences.resize(MAX_FRAME);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME; i++) {
		if (vkCreateSemaphore(m_context->GetDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_context->GetDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_context->GetDevice(), &fenceInfo, nullptr, &m_fences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores!");
		}
	}
}