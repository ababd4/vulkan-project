#include "vk_renderer.h"

#include "../Util/Util.h"
#include <array>

void Renderer::Init(VulkanContext* context, Window& window, PipelineManager* pipelineManager, AssetManager* AssetManager, Scene* scene)
{
	m_pContext = context;
	m_pPipelineManager = pipelineManager;
	m_pAssetManager = AssetManager;
	m_pScene = scene;

	m_swapchain.Init(m_pContext, window.GetWindowExtent().width, window.GetWindowExtent().height);
	m_deletionQueue.PushFunction([=, this]() {
		m_swapchain.Cleanup();
	});
	
	CreateCommandPool();
	CreateCommandBuffers();
	CreateDescriptorAllocator();
	CreateRenderPass();
	CreateFramebuffer();
	CreatePipeline();
	CreateSyncObjects();
	CreateSubmitStructures();
}

void Renderer::Cleanup()
{
	vkDeviceWaitIdle(m_pContext->GetDevice());

	m_deletionQueue.Flush();
}

void Renderer::Render()
{
	VK_CHECK(vkWaitForFences(m_pContext->GetDevice(), 1, &GetCurrentFrame().renderFence, VK_TRUE, UINT64_MAX));

	uint32_t swapchainImageIndex;
	VkResult result = vkAcquireNextImageKHR(m_pContext->GetDevice(), m_swapchain.GetSwapchain(), UINT64_MAX, GetCurrentFrame().swapchainSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

	GetCurrentFrame().deletionQueue.Flush();
	GetCurrentFrame().frameDescriptor.Clear(m_pContext->GetDevice());

	// resize swapchain
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	
	VK_CHECK(vkResetFences(m_pContext->GetDevice(), 1, &GetCurrentFrame().renderFence));
	// reset command buffer
	VK_CHECK(vkResetCommandBuffer(GetCurrentFrame().commandBuffer, 0));
	// record image index to command buffer
	RecordCommandBuffer(GetCurrentFrame().commandBuffer, swapchainImageIndex);

	// get semaphore for present from swapchain
	VkSemaphore presentSemaphore = m_swapchain.GetPresentSemaphoreByIndex(swapchainImageIndex);

	// create command buffer submit info
	VkCommandBufferSubmitInfo cmdSubmitInfo = vkinit::CreateCommandBufferSubmitInfo(GetCurrentFrame().commandBuffer);
	VkSemaphoreSubmitInfo waitInfo = vkinit::CreateSemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = vkinit::CreateSemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, presentSemaphore);

	VkSubmitInfo2 submit = vkinit::CreateSubmitInfo(&cmdSubmitInfo, &signalInfo, &waitInfo);

	// submit command buffer to the queue and execute it
	VK_CHECK(vkQueueSubmit2(m_pContext->GetGraphicsQueue(), 1, &submit, GetCurrentFrame().renderFence));

	// prepare present
	VkPresentInfoKHR presentInfo = vkinit::CreatePresentInfo();

	VkSwapchainKHR swapchains[] = { m_swapchain.GetSwapchain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &presentSemaphore;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VkResult presentResult = vkQueuePresentKHR(m_pContext->GetPresentQueue(), &presentInfo);

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
	
	// Create buffer for scene data
	AllocatedBuffer gpuSceneDataBuffer = Buffer::CreateBuffer(m_pContext->GetAllocator(), sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//add it to the deletion queue of this frame so it gets deleted once its been used
	GetCurrentFrame().deletionQueue.PushFunction([=, this]() {
		//m_pBufferManager->DestroyBuffer(gpuSceneDataBuffer);
		vmaDestroyBuffer(m_pContext->GetAllocator(), gpuSceneDataBuffer.buffer, gpuSceneDataBuffer.allocation);
	});

	//write the buffer
	void* data;
	GPUSceneData sceneData = m_pScene->GetGPUSceneData();
	vmaMapMemory(m_pContext->GetAllocator(), gpuSceneDataBuffer.allocation, &data);
	memcpy(data, &sceneData, sizeof(GPUSceneData));
	vmaUnmapMemory(m_pContext->GetAllocator(), gpuSceneDataBuffer.allocation);

	VkDescriptorSet globalDescriptor = GetCurrentFrame().frameDescriptor.Allocate(m_pContext->GetDevice(), m_gpuSceneDataDescriptorLayout);

	DescriptorWriter writer;
	writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.update_set(m_pContext->GetDevice(), globalDescriptor);

	// bind the graphics pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineManager->GetPipeline(m_pipelineDesc[0]));

	// bind descriptor set
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineManager->GetPipelineLayout(), 0, 1, &globalDescriptor, 0, nullptr);

	std::string itemName = "Suzanne";

	// submit vertex,index buffer
	GPUDrawPushConstants push_constants;
	push_constants.worldMatrix = glm::mat4{ 1.f };
	push_constants.vertexBuffer = m_pAssetManager->GetMeshByName(itemName).meshBuffers.vertexBufferAddress;

	vkCmdPushConstants(commandBuffer, m_pPipelineManager->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(commandBuffer, m_pAssetManager->GetMeshByName(itemName).meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	// render by surface
	for (GeoSurface& surface : m_pAssetManager->GetMeshByName(itemName).surfaces) {
		vkCmdDrawIndexed(commandBuffer, surface.count, 1, surface.startIndex, 0, 0);
	}

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
		VkCommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.pNext = nullptr;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = m_pContext->GetGraphicsQueueFamilyIndex();

		VK_CHECK(vkCreateCommandPool(m_pContext->GetDevice(), &commandPoolCreateInfo, nullptr, &m_frameResources[i].commandPool));
	}

	m_deletionQueue.PushFunction([this]() {
		for (int i = 0; i < MAX_FRAME; i++) {
			vkDestroyCommandPool(m_pContext->GetDevice(), m_frameResources[i].commandPool, nullptr);
		}
	});
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

	m_descriptorAllocator.Init(m_pContext->GetDevice(), 10, sizes);

	//make the descriptor set layout for draw image
	{
		std::cout << "_drawImageDescriptorLayout" << std::endl;
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		m_drawImageDescriptorLayout = builder.build(m_pContext->GetDevice(), VK_SHADER_STAGE_COMPUTE_BIT);
	}
	// gpu scene data
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_gpuSceneDataDescriptorLayout = builder.build(m_pContext->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	m_drawImageDescriptors = m_descriptorAllocator.Allocate(m_pContext->GetDevice(), m_drawImageDescriptorLayout);

	{
		DescriptorWriter writer;
		writer.write_image(0, m_swapchain.GetDrawImage().imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		writer.update_set(m_pContext->GetDevice(), m_drawImageDescriptors);
	}

	for (int i = 0; i < MAX_FRAME; i++) {
		// create a descriptor pool
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		m_frameResources[i].frameDescriptor = DescriptorAllocatorGrowable{};
		m_frameResources[i].frameDescriptor.Init(m_pContext->GetDevice(), 1000, frame_sizes);
	}

	m_deletionQueue.PushFunction([this]() {
		m_descriptorAllocator.Clear(m_pContext->GetDevice());
		vkDestroyDescriptorSetLayout(m_pContext->GetDevice(), m_drawImageDescriptorLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_pContext->GetDevice(), m_gpuSceneDataDescriptorLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_pContext->GetDevice(), m_layout, nullptr);
	});
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

	m_deletionQueue.PushFunction([this] {
		vkDestroyRenderPass(m_pContext->GetDevice(), m_renderPass, nullptr);
	});
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

	m_deletionQueue.PushFunction([this]() {
		for (int i = 0; i < m_framebuffers.size(); i++) {
			vkDestroyFramebuffer(m_pContext->GetDevice(), m_framebuffers[i], nullptr);
		}
	});
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

	m_deletionQueue.PushFunction([this]() {
		for (int i = 0; i < MAX_FRAME; i++) {
			//destroy sync objects
			vkDestroyFence(m_pContext->GetDevice(), m_frameResources[i].renderFence, nullptr);
			vkDestroySemaphore(m_pContext->GetDevice(), m_frameResources[i].renderSemaphore, nullptr);
			vkDestroySemaphore(m_pContext->GetDevice(), m_frameResources[i].swapchainSemaphore, nullptr);

			m_frameResources[i].deletionQueue.Flush();
			m_frameResources[i].frameDescriptor.Clear(m_pContext->GetDevice());
		}
	});
}

void Renderer::CreateSubmitStructures()
{
	VkCommandPoolCreateInfo immCommandPoolCreateInfo{};
	immCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	immCommandPoolCreateInfo.pNext = nullptr;
	immCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	immCommandPoolCreateInfo.queueFamilyIndex = m_pContext->GetGraphicsQueueFamilyIndex();
	VK_CHECK(vkCreateCommandPool(m_pContext->GetDevice(), &immCommandPoolCreateInfo, nullptr, &m_immCommandPool));

	VkCommandBufferAllocateInfo immAllocInfo{};
	immAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	immAllocInfo.commandPool = m_immCommandPool;
	immAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	immAllocInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(m_pContext->GetDevice(), &immAllocInfo, &m_immCommandBuffer));

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VK_CHECK(vkCreateFence(m_pContext->GetDevice(), &fenceInfo, nullptr, &m_immFence));

	m_deletionQueue.PushFunction([this]() {
		vkDestroyCommandPool(m_pContext->GetDevice(), m_immCommandPool, nullptr);
		vkDestroyFence(m_pContext->GetDevice(), m_immFence, nullptr);
	});
}

void Renderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(m_pContext->GetDevice(), 1, &m_immFence));
	VK_CHECK(vkResetCommandBuffer(m_immCommandBuffer, 0));

	VkCommandBuffer cmd = m_immCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::CreateCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = vkinit::CreateCommandBufferSubmitInfo(cmd);
	VkSubmitInfo2 submit = vkinit::CreateSubmitInfo(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  m_immFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(m_pContext->GetGraphicsQueue(), 1, &submit, m_immFence));

	VK_CHECK(vkWaitForFences(m_pContext->GetDevice(), 1, &m_immFence, true, 9999999999));
}