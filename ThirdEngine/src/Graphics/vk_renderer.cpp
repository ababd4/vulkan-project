#include "vk_renderer.h"

#include "../Util/Util.h"
#include <array>

void Renderer::Init(VulkanContext* context, Window& window, PipelineManager* pipelineManager)
{
	m_pContext = context;
	m_pPipelineManager = pipelineManager;
	m_buffer.Init(*context);
	m_swapchain.Init(m_pContext, window.GetWindowExtent().width, window.GetWindowExtent().height);

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
	vkDeviceWaitIdle(m_pContext->GetDevice());

	m_buffer.Cleanup();

	for (int i = 0; i < m_framebuffers.size(); i++) {
		vkDestroyFramebuffer(m_pContext->GetDevice(), m_framebuffers[i], nullptr);
	}

	for (int i = 0; i < MAX_FRAME; i++) {
		vkDestroyCommandPool(m_pContext->GetDevice(), m_frameResources[i].commandPool, nullptr);

		//destroy sync objects
		vkDestroyFence(m_pContext->GetDevice(), m_frameResources[i].renderFence, nullptr);
		vkDestroySemaphore(m_pContext->GetDevice(), m_frameResources[i].renderSemaphore, nullptr);
		vkDestroySemaphore(m_pContext->GetDevice(), m_frameResources[i].swapchainSemaphore, nullptr);
	}

	m_swapchain.Cleanup();
	m_descriptorAllocator.clear(m_pContext->GetDevice());
	vkDestroyDescriptorSetLayout(m_pContext->GetDevice(), m_drawImageDescriptorLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_pContext->GetDevice(), m_layout, nullptr);
	vkDestroyRenderPass(m_pContext->GetDevice(), m_renderPass, nullptr);
}

void Renderer::UpdateScene()
{

}

void Renderer::Render()
{
	VK_CHECK(vkWaitForFences(m_pContext->GetDevice(), 1, &GetCurrentFrame().renderFence, VK_TRUE, UINT64_MAX));

	uint32_t swapchainImageIndex;
	VkResult result = vkAcquireNextImageKHR(m_pContext->GetDevice(), m_swapchain.GetSwapchain(), UINT64_MAX, GetCurrentFrame().swapchainSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

	// resize swapchain
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	// reset command buffer
	VK_CHECK(vkResetFences(m_pContext->GetDevice(), 1, &GetCurrentFrame().renderFence));

	VK_CHECK(vkResetCommandBuffer(GetCurrentFrame().commandBuffer, 0));
	// record image index to command buffer
	RecordCommandBuffer(GetCurrentFrame().commandBuffer, swapchainImageIndex);

	// create command buffer submit info
	VkCommandBufferSubmitInfo cmdSubmitInfo = vkinit::CreateCommandBufferSubmitInfo(GetCurrentFrame().commandBuffer);
	VkSemaphoreSubmitInfo waitInfo = vkinit::CreateSemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = vkinit::CreateSemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().renderSemaphore);

	VkSubmitInfo2 submit = vkinit::CreateSubmitInfo(&cmdSubmitInfo, &signalInfo, &waitInfo);

	// submit command buffer to the queue and execute it
	VK_CHECK(vkQueueSubmit2(m_pContext->GetGraphicsQueue(), 1, &submit, GetCurrentFrame().renderFence));

	// prepare present
	VkPresentInfoKHR presentInfo = vkinit::CreatePresentInfo();

	VkSwapchainKHR swapchains[] = { m_swapchain.GetSwapchain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &GetCurrentFrame().renderSemaphore;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VkResult presentResult = vkQueuePresentKHR(m_pContext->GetGraphicsQueue(), &presentInfo);

	// increase frame index
	currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAME;
}

void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo = vkinit::CreateCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	VkClearValue clear{};
	clear.color = { {0.f, 0.f, 1.f, 1.f} };
	VkRenderPassBeginInfo renderPassInfo = vkinit::CreateRenderPassBeginInfo(m_renderPass, m_framebuffers[imageIndex], 0, 0, m_swapchain.GetSwapchainExtent(), &clear);

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind the graphics pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineManager->GetPipeline(m_pipelineDesc[0]));

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

void Renderer::RecreateSwapchain()
{
	vkDeviceWaitIdle(m_pContext->GetDevice());

	m_swapchain.Cleanup();
	// destroy framebuffer
	for (int i = 0; i < m_framebuffers.size(); i++) {
		vkDestroyFramebuffer(m_pContext->GetDevice(), m_framebuffers[i], nullptr);
	}

	m_swapchain.Init(m_pContext, m_swapchain.GetSwapchainExtent().width, m_swapchain.GetSwapchainExtent().height);
	CreateFramebuffer();
}

void Renderer::CreateCommandPool()
{
	for (int i = 0; i < MAX_FRAME; i++) {
		VkCommandPoolCreateInfo command_pool_create_info{};
		command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_create_info.pNext = nullptr;
		command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		command_pool_create_info.queueFamilyIndex = m_pContext->GetQueueFamilyIndex();

		VK_CHECK(vkCreateCommandPool(m_pContext->GetDevice(), &command_pool_create_info, nullptr, &m_frameResources[i].commandPool));
	}
}

void Renderer::CreateCommandBuffers()
{
	for (int i = 0; i < MAX_FRAME; i++) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_frameResources[i].commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VK_CHECK(vkAllocateCommandBuffers(m_pContext->GetDevice(), &allocInfo, &m_frameResources[i].commandBuffer));
	}
}

void Renderer::CreateDescriptorAllocator()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
	};

	m_descriptorAllocator.init(m_pContext->GetDevice(), 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		std::cout << "_drawImageDescriptorLayout" << std::endl;
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		m_drawImageDescriptorLayout = builder.build(m_pContext->GetDevice(), VK_SHADER_STAGE_COMPUTE_BIT);
	}

	m_drawImageDescriptors = m_descriptorAllocator.allocate(m_pContext->GetDevice(), m_drawImageDescriptorLayout);

	{
		DescriptorWriter writer;
		writer.write_image(0, m_swapchain.GetDrawImage().imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		writer.update_set(m_pContext->GetDevice(), m_drawImageDescriptors);
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

	VK_CHECK( vkCreateRenderPass(m_pContext->GetDevice(), &renderPassInfo, nullptr, &m_renderPass) );
}

void Renderer::CreateFramebuffer()
{
	m_framebuffers.resize(m_swapchain.GetSwapchainImageViews().size());

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

		VK_CHECK(vkCreateFramebuffer(m_pContext->GetDevice(), &framebufferCreateInfo, nullptr, &m_framebuffers[i]));
	}
}

void Renderer::CreatePipeline()
{
	PipelineDesc triangleDesc;
	triangleDesc.vert = "res/shaders/mesh.vert.spv";
	triangleDesc.frag = "res/shaders/mesh.frag.spv";
	triangleDesc.renderPass = m_renderPass;
	triangleDesc.subpass = 0;

	m_pipelineDesc.push_back(triangleDesc);
}

void Renderer::CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME; i++) {
		VK_CHECK(vkCreateSemaphore(m_pContext->GetDevice(), &semaphoreInfo, nullptr, &m_frameResources[i].swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(m_pContext->GetDevice(), &semaphoreInfo, nullptr, &m_frameResources[i].renderSemaphore));
		VK_CHECK(vkCreateFence(m_pContext->GetDevice(), &fenceInfo, nullptr, &m_frameResources[i].renderFence));
	}
}