#include "Renderer.h"

#include "Util/Util.h"
#include <array>

void Renderer::Init(VulkanContext* context, Window& window, AssetManager* AssetManager, Scene* scene, ImGuiLayer* imGuiLayer, EngineStats* stats)
{
	m_pContext = context;
	m_pAssetManager = AssetManager;
	m_pScene = scene;
	m_pImGuiLayer = imGuiLayer;
	m_pStats = stats;

	m_swapchain.Init(m_pContext, window.GetWindowExtent().width, window.GetWindowExtent().height);
	m_deletionQueue.PushFunction([this]() {
		m_swapchain.Cleanup();
	});
	
	CreateCommandPool();
	CreateCommandBuffers();
	CreateImages(window.GetWindowExtent().width, window.GetWindowExtent().height);
	CreateDescriptorAllocator();
	//CreateRenderPass();
	//CreateFramebuffer();
	CreateSyncObjects();
	CreateSubmitStructures();

	m_pImGuiLayer->Init(m_pContext, &m_swapchain, &window);
	m_deletionQueue.PushFunction([=, this]() {
		m_pImGuiLayer->Cleanup();
	});

	m_shadowResources.Init(m_pContext, this);
	m_deletionQueue.PushFunction([this]() {
		m_shadowResources.Cleanup(m_pContext);
	});
}

void Renderer::Cleanup()
{
	m_deletionQueue.Flush();
}

bool IsVisible(const RenderObject& obj, const glm::mat4& viewproj) {
	std::array<glm::vec3, 8> corners{
		glm::vec3 { 1, 1, 1 },
		glm::vec3 { 1, 1, -1 },
		glm::vec3 { 1, -1, 1 },
		glm::vec3 { 1, -1, -1 },
		glm::vec3 { -1, 1, 1 },
		glm::vec3 { -1, 1, -1 },
		glm::vec3 { -1, -1, 1 },
		glm::vec3 { -1, -1, -1 },
	};

	glm::mat4 matrix = viewproj * obj.transform;

	glm::vec3 min = { 1.5, 1.5, 1.5 };
	glm::vec3 max = { -1.5, -1.5, -1.5 };

	for (int c = 0; c < 8; c++) {
		// project each corner into clip space
		glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

		// perspective correction
		v.x = v.x / v.w;
		v.y = v.y / v.w;
		v.z = v.z / v.w;

		min = glm::min(glm::vec3{ v.x, v.y, v.z }, min);
		max = glm::max(glm::vec3{ v.x, v.y, v.z }, max);
	}

	// check the clip space box is within the view
	if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
		return false;
	}
	else {
		return true;
	}
}

void Renderer::Render(ImGuiLayer& imguiLayer)
{
	imguiLayer.BeginFrame(m_pStats, m_pScene->GetGPUSceneData().cameraPos);

	// reset stats
	m_pStats->drawcallCount = 0;
	m_pStats->triangleCount = 0;
	// begin clock
	auto start = std::chrono::system_clock::now();
	
	// wait for the previous frame process to complete
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
	
	m_drawExtent.height = std::min(m_swapchain.GetSwapchainExtent().height, m_drawImage.imageExtent.height) * renderScale;
	m_drawExtent.width = std::min(m_swapchain.GetSwapchainExtent().width, m_drawImage.imageExtent.width) * renderScale;

	VK_CHECK(vkResetFences(m_pContext->GetDevice(), 1, &GetCurrentFrame().renderFence));

	VkCommandBuffer cmd = GetCurrentFrame().commandBuffer;

	// reset command buffer
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	VkCommandBufferBeginInfo beginInfo = vkinit::CreateCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

	// Create buffer for scene data
	AllocatedBuffer gpuSceneDataBuffer = Buffer::CreateBuffer(m_pContext->GetAllocator(), sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, false, "gpuSceneData");

	//add it to the deletion queue of this frame so it gets deleted once its been used
	GetCurrentFrame().deletionQueue.PushFunction([=, this]() {
		//vmaDestroyBuffer(m_pContext->GetAllocator(), gpuSceneDataBuffer.buffer, gpuSceneDataBuffer.allocation);
		Buffer::DestroyBuffer(m_pContext->GetAllocator(), gpuSceneDataBuffer, false, "gpuSceneData");
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

	DrawShadowPass(cmd, globalDescriptor);

	// transit drawImage from UNDEFINED into GENERAL so that we can draw it
	vkutil::TransitionImage(cmd, m_drawImage.image, m_drawImage.imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// record image index to command buffer
	DrawMainPass(cmd, swapchainImageIndex, globalDescriptor);

	vkutil::TransitionImage(cmd, m_drawImage.image, m_drawImage.imageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::TransitionImage(cmd, m_swapchain.GetSwapchainImages()[swapchainImageIndex], m_swapchain.GetSwapchainImageFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	vkutil::CopyImageToImage(cmd, m_drawImage.image, m_swapchain.GetSwapchainImages()[swapchainImageIndex], m_drawExtent, m_swapchain.GetSwapchainExtent());

	vkutil::TransitionImage(cmd, m_swapchain.GetSwapchainImages()[swapchainImageIndex], m_swapchain.GetSwapchainImageFormat(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	// draw imgui into swapchain image directly
	imguiLayer.Render(cmd, m_swapchain.GetSwapchainImageViews()[swapchainImageIndex], m_swapchain.GetSwapchainExtent());

	vkutil::TransitionImage(cmd, m_swapchain.GetSwapchainImages()[swapchainImageIndex], m_swapchain.GetSwapchainImageFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSemaphore presentSemaphore = m_swapchain.GetPresentSemaphoreByIndex(swapchainImageIndex);

	// create command buffer submit info
	VkCommandBufferSubmitInfo cmdSubmitInfo = vkinit::CreateCommandBufferSubmitInfo(cmd);
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

	// get clock again and compare with start clock
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	m_pStats->meshDrawTime = elapsed.count() / 1000.0f;

	// increase frame index
	currentFrameIndex++;
}

void Renderer::DrawMainPass(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkDescriptorSet globalDescriptor)
{
	std::vector<uint32_t> opaqueDraws;
	opaqueDraws.reserve(m_pScene->mainDrawContext.OpaqueSurfaces.size());

	for (uint32_t i = 0; i < m_pScene->mainDrawContext.OpaqueSurfaces.size(); i++) {
		//if (IsVisible(m_pScene->mainDrawContext.OpaqueSurfaces[i], m_pScene->GetGPUSceneData().viewproj)) {
		//	opaqueDraws.push_back(i);
		//}

		// WARNING: we don't check visibility
		opaqueDraws.push_back(i);
	}

	// sort the opaque surfaces by material and mesh
	std::sort(opaqueDraws.begin(), opaqueDraws.end(), [&](const auto& iA, const auto& iB) {
		const RenderObject& A = m_pScene->mainDrawContext.OpaqueSurfaces[iA];
		const RenderObject& B = m_pScene->mainDrawContext.OpaqueSurfaces[iB];
		if (A.material == B.material) {
			return A.indexBuffer < B.indexBuffer;
		}
		else {
			return A.material < B.material;
		}
	});

	vkutil::TransitionImage(commandBuffer, m_drawImage.image, m_drawImage.imageFormat, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vkutil::TransitionImage(commandBuffer, m_depthImage.image, m_depthImage.imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkClearValue colorClearValue = {0.f, 0.f, 0.f, 1.f};
	VkClearDepthStencilValue depthClearValue = { 1.f, 0 };

	VkRenderingAttachmentInfo colorAttachment = vkinit::CreateColorAttachmentInfo(m_drawImage.imageView, &colorClearValue, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingAttachmentInfo depthAttachment = vkinit::CreateDepthAttachmentInfo(m_depthImage.imageView, &depthClearValue, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkExtent2D extent = { m_swapchain.GetSwapchainExtent() };
	VkRenderingInfo renderingInfo = vkinit::CreateRenderingInfo(extent, &colorAttachment, &depthAttachment);

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	MaterialPipeline* lastPipeline = nullptr;
	MaterialInstance* lastMaterial = nullptr;
	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

	auto draw = [&](const RenderObject& draw) {
		if (draw.material != lastMaterial) {
			lastMaterial = draw.material;

			if (draw.material->pipeline != lastPipeline) {
				lastPipeline = draw.material->pipeline;

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

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
			}

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 1, 1, &draw.material->materialSet, 0, nullptr);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 2, 1, &m_shadowResources.descriptorSet, 0, nullptr);
		}

		if (draw.indexBuffer != lastIndexBuffer) {
			lastIndexBuffer = draw.indexBuffer;
			vkCmdBindIndexBuffer(commandBuffer, draw.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}

		GPUDrawPushConstants pushConstants;
		pushConstants.vertexBuffer = draw.vertexBufferAddress;
		pushConstants.worldMatrix = draw.transform;
		vkCmdPushConstants(commandBuffer, draw.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);

		vkCmdDrawIndexed(commandBuffer, draw.indexCount, 1, draw.firstIndex, 0, 0);

		// stats
		m_pStats->drawcallCount++;
		m_pStats->triangleCount += draw.indexCount / 3;
	};

	for (auto& r : opaqueDraws) {
		draw(m_pScene->mainDrawContext.OpaqueSurfaces[r]);
	}

	for (auto& r : m_pScene->mainDrawContext.TransparentSurfaces) {
		draw(r);
	}


	// render pass can be ended
	//vkCmdEndRenderPass(commandBuffer);

	vkCmdEndRendering(commandBuffer);

	m_pScene->mainDrawContext.OpaqueSurfaces.clear();
	m_pScene->mainDrawContext.TransparentSurfaces.clear();
}

void Renderer::RecreateSwapchain()
{
	vkDeviceWaitIdle(m_pContext->GetDevice());

	m_swapchain.Cleanup();
	// destroy framebuffer
	//for (int i = 0; i < m_framebuffers.size(); i++) {
	//	vkDestroyFramebuffer(m_pContext->GetDevice(), m_framebuffers[i], nullptr);
	//}

	m_swapchain.Init(m_pContext, m_swapchain.GetSwapchainExtent().width, m_swapchain.GetSwapchainExtent().height);
	//CreateFramebuffer();
}

void Renderer::DrawShadowPass(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor)
{
	// transit shadowImage so that i can record depth
	vkutil::TransitionImage(
		cmd, 
		m_shadowResources.shadowImage.image, 
		m_shadowResources.shadowImage.imageFormat, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
	);

	std::vector<uint32_t> opaqueDraws;
	opaqueDraws.reserve(m_pScene->mainDrawContext.OpaqueSurfaces.size());

	for (uint32_t i = 0; i < m_pScene->mainDrawContext.OpaqueSurfaces.size(); i++) {
		if (IsVisible(m_pScene->mainDrawContext.OpaqueSurfaces[i], m_pScene->GetGPUSceneData().lightViewProj)) {
			opaqueDraws.push_back(i);
		}
	}

	// sort the opaque surfaces by material and mesh
	std::sort(opaqueDraws.begin(), opaqueDraws.end(), [&](const auto& iA, const auto& iB) {
		const RenderObject& A = m_pScene->mainDrawContext.OpaqueSurfaces[iA];
		const RenderObject& B = m_pScene->mainDrawContext.OpaqueSurfaces[iB];
		if (A.material == B.material) {
			return A.indexBuffer < B.indexBuffer;
		}
		else {
			return A.material < B.material;
		}
	});

	VkClearDepthStencilValue depthClearValue = { 1.f, 0 };
	VkRenderingAttachmentInfo depthAttachment = vkinit::CreateDepthAttachmentInfo(m_shadowResources.shadowImage.imageView, &depthClearValue, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkExtent2D extent = { m_shadowResources.extent.width, m_shadowResources.extent.height };
	VkRenderingInfo renderingInfo = vkinit::CreateRenderingInfo(extent, nullptr, &depthAttachment);

	vkCmdBeginRendering(cmd, &renderingInfo);

	// Set viewport and sicissor values
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_shadowResources.extent.width);
	viewport.height = static_cast<float>(m_shadowResources.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { m_shadowResources.extent.width, m_shadowResources.extent.height };
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowResources.pipeline);
	vkCmdSetDepthBias(cmd, 1.25f, 0.0f, 1.75f);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowResources.layout, 0, 1, &globalDescriptor, 0, nullptr);

	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

	auto draw = [&](const RenderObject& draw) {
		if (draw.indexBuffer != lastIndexBuffer) {
			lastIndexBuffer = draw.indexBuffer;
			vkCmdBindIndexBuffer(cmd, draw.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}

		GPUDrawPushConstants pushConstants;
		pushConstants.vertexBuffer = draw.vertexBufferAddress;
		pushConstants.worldMatrix = draw.transform;
		vkCmdPushConstants(cmd, m_shadowResources.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);

		vkCmdDrawIndexed(cmd, draw.indexCount, 1, draw.firstIndex, 0, 0);
		m_pStats->drawcallCount++;
		m_pStats->triangleCount += draw.indexCount / 3;
	};

	for (auto& r : opaqueDraws) {
		draw(m_pScene->mainDrawContext.OpaqueSurfaces[r]);
	}

	//for (auto& r : m_pScene->mainDrawContext.TransparentSurfaces) {
	//	draw(r);
	//}

	vkCmdEndRendering(cmd);

	vkutil::TransitionImage(
		cmd,
		m_shadowResources.shadowImage.image,
		m_shadowResources.shadowImage.imageFormat,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
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

void Renderer::CreateImages(uint32_t width, uint32_t height)
{
	//draw image size will match the window
	VkExtent3D drawImageExtent = {
		width,
		height,
		1
	};

	//hardcoding the draw format to 32 bit float
	m_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	m_drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info = vkinit::CreateImageCreateInfo(m_drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(m_pContext->GetAllocator(), &rimg_info, &rimg_allocinfo, &m_drawImage.image, &m_drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = vkinit::CreateImageviewCreateInfo(m_drawImage.imageFormat, m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(m_pContext->GetDevice(), &rview_info, nullptr, &m_drawImage.imageView));

	m_depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
	m_depthImage.imageExtent = drawImageExtent;
	VkImageUsageFlags depthImageUsages{};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkImageCreateInfo dimg_info = vkinit::CreateImageCreateInfo(m_depthImage.imageFormat, depthImageUsages, drawImageExtent);

	//allocate and create the image
	vmaCreateImage(m_pContext->GetAllocator(), &dimg_info, &rimg_allocinfo, &m_depthImage.image, &m_depthImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo dview_info = vkinit::CreateImageviewCreateInfo(m_depthImage.imageFormat, m_depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(m_pContext->GetDevice(), &dview_info, nullptr, &m_depthImage.imageView));

	m_deletionQueue.PushFunction([this]() {
		vkDestroyImageView(m_pContext->GetDevice(), m_drawImage.imageView, nullptr);
		vmaDestroyImage(m_pContext->GetAllocator(), m_drawImage.image, m_drawImage.allocation);

		vkDestroyImageView(m_pContext->GetDevice(), m_depthImage.imageView, nullptr);
		vmaDestroyImage(m_pContext->GetAllocator(), m_depthImage.image, m_depthImage.allocation);
	});
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
		// std::cout << "_drawImageDescriptorLayout" << std::endl;
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
		writer.write_image(0, m_drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

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

//void Renderer::CreateRenderPass()
//{
//	VkAttachmentDescription colorAttachment{};
//	colorAttachment.format = m_swapchain.GetSwapchainImageFormat();
//	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//	VkAttachmentReference colorAttachmentRef{};
//	colorAttachmentRef.attachment = 0;
//	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentDescription depthAttachment{};
//	depthAttachment.format = m_swapchain.GetDepthImage().imageFormat;
//	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentReference depthAttachmentRef{};
//	depthAttachmentRef.attachment = 1;
//	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//	VkSubpassDescription subpass{};
//	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//
//	subpass.colorAttachmentCount = 1;
//	subpass.pColorAttachments = &colorAttachmentRef;
//	subpass.pDepthStencilAttachment = &depthAttachmentRef;
//
//	VkAttachmentDescription attachments[] = {
//		colorAttachment,
//		depthAttachment
//	};
//
//	VkRenderPassCreateInfo renderPassInfo{};
//	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//	renderPassInfo.attachmentCount = 2;
//	renderPassInfo.pAttachments = attachments;
//	renderPassInfo.subpassCount = 1;
//	renderPassInfo.pSubpasses = &subpass;
//
//	VK_CHECK( vkCreateRenderPass(m_pContext->GetDevice(), &renderPassInfo, nullptr, &m_renderPass) );
//
//	m_deletionQueue.PushFunction([this] {
//		vkDestroyRenderPass(m_pContext->GetDevice(), m_renderPass, nullptr);
//	});
//}

//void Renderer::CreateFramebuffer()
//{
//	m_framebuffers.resize(m_swapchain.GetSwapchainColorImageViews().size());
//
//	for (size_t i = 0; i < m_swapchain.GetSwapchainColorImageViews().size(); i++) {
//		std::array<VkImageView, 2> attachments = {
//				m_swapchain.GetSwapchainColorImageViews()[i],
//				// Feature: add depth imageview
//				m_swapchain.GetDepthImage().imageView
//		};
//
//		VkFramebufferCreateInfo framebufferCreateInfo{};
//		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//		framebufferCreateInfo.width = m_swapchain.GetSwapchainExtent().width;
//		framebufferCreateInfo.height = m_swapchain.GetSwapchainExtent().height;
//		framebufferCreateInfo.layers = 1;
//		framebufferCreateInfo.renderPass = m_renderPass;
//		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//		framebufferCreateInfo.pAttachments = attachments.data();
//
//		VK_CHECK(vkCreateFramebuffer(m_pContext->GetDevice(), &framebufferCreateInfo, nullptr, &m_framebuffers[i]));
//	}
//
//	m_deletionQueue.PushFunction([this]() {
//		for (int i = 0; i < m_framebuffers.size(); i++) {
//			vkDestroyFramebuffer(m_pContext->GetDevice(), m_framebuffers[i], nullptr);
//		}
//	});
//}

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