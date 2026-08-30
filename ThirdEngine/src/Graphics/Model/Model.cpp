#include "../Model/Model.h"
#include "../Buffer/Buffer.h"
#include "../Pipeline/PipelineBuilder.h"
#include "../Asset/AssetManager.h"
#include "../Renderer.h"

void GLTF::Model::ClearAll()
{
	//std::cout << "image count = " << images.size() << std::endl;

	descriptorPool.DestroyPools(m_pContext->GetDevice());
	//vmaDestroyBuffer(m_pContext->GetAllocator(), materialDataBuffer.buffer, materialDataBuffer.allocation);
	Buffer::DestroyBuffer(m_pContext->GetAllocator(), materialDataBuffer, false, "materialData");

	for (auto& [k, v] : meshes)
	{
		//vmaDestroyBuffer(m_pContext->GetAllocator(), v->meshBuffers.indexBuffer.buffer, v->meshBuffers.indexBuffer.allocation);
		//vmaDestroyBuffer(m_pContext->GetAllocator(), v->meshBuffers.vertexBuffer.buffer, v->meshBuffers.vertexBuffer.allocation);

		Buffer::DestroyBuffer(m_pContext->GetAllocator(), v->meshBuffers.indexBuffer, false, "index");
		Buffer::DestroyBuffer(m_pContext->GetAllocator(), v->meshBuffers.vertexBuffer, false, "vertex");
	}

	for (auto& image : images) {

		if (image.image == m_pAssetManager->GetErrorImage().image) {
			//dont destroy the default images
			continue;
		}
		m_pAssetManager->DestroyImage(image);
	}

	for (auto& sampler : samplers) {
		vkDestroySampler(m_pContext->GetDevice(), sampler, nullptr);
	}
}

void GLTF::MaterialSystem::Init(VkDevice device, Swapchain swapchain, Renderer* renderer, std::string vert, std::string frag)
{
	//VkAttachmentDescription colorAttachment{};
	//colorAttachment.format = swapchain.GetSwapchainImageFormat();
	//colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	//colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	//colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	//colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//VkAttachmentReference colorAttachmentRef{};
	//colorAttachmentRef.attachment = 0;
	//colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//VkAttachmentDescription depthAttachment{};
	//depthAttachment.format = swapchain.GetDepthImage().imageFormat;
	//depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	//depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	//depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	//depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//VkAttachmentReference depthAttachmentRef{};
	//depthAttachmentRef.attachment = 1;
	//depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//VkSubpassDescription subpass{};
	//subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	//subpass.colorAttachmentCount = 1;
	//subpass.pColorAttachments = &colorAttachmentRef;
	//subpass.pDepthStencilAttachment = &depthAttachmentRef;

	//VkAttachmentDescription attachments[] = {
	//	colorAttachment,
	//	depthAttachment
	//};

	//VkRenderPassCreateInfo renderPassInfo{};
	//renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	//renderPassInfo.attachmentCount = 2;
	//renderPassInfo.pAttachments = attachments;
	//renderPassInfo.subpassCount = 1;
	//renderPassInfo.pSubpasses = &subpass;	

	//VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

	VkFormat colorFormat = renderer->GetDrawImage().imageFormat;
	VkFormat depthFormat = renderer->GetDepthImage().imageFormat;

	VkPipelineRenderingCreateInfo renderingCreateInfo{};
	renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingCreateInfo.colorAttachmentCount = 1;
	renderingCreateInfo.pColorAttachmentFormats = &colorFormat;
	renderingCreateInfo.depthAttachmentFormat = depthFormat;
	renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

	VkShaderModule meshFragShader;
	if (!vkutil::LoadShaderModule(frag, device, &meshFragShader)) {
		fmt::println("Error when building the triangle fragment shader module");
	}

	VkShaderModule meshVertexShader;
	if (!vkutil::LoadShaderModule(vert, device, &meshVertexShader)) {
		fmt::println("Error when building the triangle vertex shader module");
	}

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.add_binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	materialLayout = layoutBuilder.build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layouts[] = { renderer->GetGlobalDescriptorSetLayout(), materialLayout, renderer->GetShadowDescriptorSetLayout()};

	VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::CreatePipelineLayoutCreateInfo();
	mesh_layout_info.setLayoutCount = 3;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(device, &mesh_layout_info, nullptr, &newLayout));

	opaquePipeline.layout = newLayout;
	transparentPipeline.layout = newLayout;

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.disable_blending();
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_LESS);

	pipelineBuilder.set_color_attachment_format(renderer->GetDrawImage().imageFormat);
	pipelineBuilder.set_depth_format(renderer->GetDepthImage().imageFormat);

	pipelineBuilder._pipelineLayout = newLayout;
	opaquePipeline.pipeline = pipelineBuilder.BuildPipeline(device);

	pipelineBuilder.enable_blending_alphablend();
	pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_LESS);

	transparentPipeline.pipeline = pipelineBuilder.BuildPipeline(device);

	vkDestroyShaderModule(device, meshFragShader, nullptr);
	vkDestroyShaderModule(device, meshVertexShader, nullptr);
}

void GLTF::MaterialSystem::ClearResources(VkDevice device)
{
	vkDestroyPipeline(device, opaquePipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(device, opaquePipeline.layout, nullptr);
	vkDestroyPipeline(device, transparentPipeline.pipeline, nullptr);

	vkDestroyDescriptorSetLayout(device, materialLayout, nullptr);
}

MaterialInstance GLTF::MaterialSystem::WriteMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
{
	MaterialInstance matData;
	matData.passType = pass;
	if (pass == MaterialPass::Transparent) {
		matData.pipeline = &transparentPipeline;
	}
	else {
		matData.pipeline = &opaquePipeline;
	}

	matData.materialSet = descriptorAllocator.Allocate(device, materialLayout);

	writer.clear();
	writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.write_image(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.write_image(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.write_image(3, resources.normalImage.imageView, resources.normalSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.update_set(device, matData.materialSet);

	return matData;
}


void GLTF::MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
	glm::mat4 nodeMatrix = topMatrix * worldTransform;

	for (auto& s : mesh->surfaces) {
		RenderObject def;
		def.indexCount = s.count;
		def.firstIndex = s.startIndex;
		def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
		def.material = &s.material->data;
		def.bounds = s.bounds;
		def.transform = nodeMatrix;
		def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

		if (s.material->data.passType == MaterialPass::Transparent) {
			ctx.TransparentSurfaces.push_back(def);
		}
		else {
			ctx.OpaqueSurfaces.push_back(def);
		}
		
	}

	// recurse down
	Node::Draw(topMatrix, ctx);
}

void GLTF::Model::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
	// create renderables from the scenenodes
	for (auto& n : topNodes) {
		n->Draw(topMatrix, ctx);
	}
}