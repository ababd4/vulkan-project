#include "../Asset/AssetManager.h"

#include "Util/Util.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/Backend/Swapchain.h"
#include "Graphics/Pipeline/PipelineBuilder.h"
#include "../Loader/HDRLoader.h"

#include <array>

void AssetManager::Init(VulkanContext* context, Renderer* renderer)
{
	m_pContext = context;
	m_pRenderer = renderer;
}

void AssetManager::Cleanup()
{
	//for (auto& [name, mesh] : m_meshes) {
	//	Buffer::DestroyBuffer(m_pContext->GetAllocator(), mesh.meshBuffers.vertexBuffer, false, "vertex");
	//	Buffer::DestroyBuffer(m_pContext->GetAllocator(), mesh.meshBuffers.indexBuffer, false, "index");
	//}
	//m_meshes.clear();
	m_gltfModels.clear();

	m_deletionQueue.Flush();
}

void AssetManager::InitMaterials(Swapchain& swapchain)
{
	GLTF::MaterialSystem ms;
	m_materialSystems[ShadingType::Phong] = ms;
	m_materialSystems[ShadingType::PBR] = ms;

	m_materialSystems[ShadingType::Phong].Init(m_pContext->GetDevice(), swapchain, m_pRenderer, "res/shaders/Phong.vert.spv", "res/shaders/Phong.frag.spv");
	m_materialSystems[ShadingType::PBR].Init(m_pContext->GetDevice(), swapchain, m_pRenderer, "res/shaders/PBR.vert.spv", "res/shaders/PBR.frag.spv");

	m_deletionQueue.PushFunction([this] {
		m_materialSystems[ShadingType::Phong].ClearResources(m_pContext->GetDevice());
		m_materialSystems[ShadingType::PBR].ClearResources(m_pContext->GetDevice());
	});
}

void AssetManager::LoadModels()
{
	LoadGltfModel("cube", "res/models/cube.glb", ShadingType::Phong);
	LoadGltfModel("floor", "res/models/floor.glb", ShadingType::Phong);
	LoadGltfModel("case", "res/models/case.glb", ShadingType::Phong);
	LoadGltfModel("teapot1", "res/models/utah_teapot.glb", ShadingType::Phong);
	LoadGltfModel("teapot2", "res/models/utah_teapot.glb", ShadingType::Phong);
	//LoadGltfModel("structure", "res/models/structure.glb", ShadingType::Phong);
	LoadGltfModel("helmet", "res/models/DamagedHelmet/DamagedHelmet.gltf", ShadingType::PBR);
	LoadGltfModel("sponza", "res/models/Sponza/Sponza.gltf", ShadingType::PBR);
}

GPUMeshBuffers AssetManager::UploadMesh(Renderer* renderer, std::string name, std::span<uint32_t> indices, std::span<Vertex> vertices)
{
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	GPUMeshBuffers newSurface;

	//create vertex buffer
	newSurface.vertexBuffer = Buffer::CreateBuffer(
		m_pContext->GetAllocator(),
		vertexBufferSize, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		false,
		"vertex"
	);

	//find the adress of the vertex buffer
	VkBufferDeviceAddressInfo deviceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
	newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(m_pContext->GetDevice(), &deviceAddressInfo);
	

	//create index buffer
	newSurface.indexBuffer = Buffer::CreateBuffer(
		m_pContext->GetAllocator(),
		indexBufferSize, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		false,
		"index"
	);

	AllocatedBuffer staging = Buffer::CreateStagingBuffer(m_pContext->GetAllocator(), vertexBufferSize + indexBufferSize);

	void* data;
	vmaMapMemory(m_pContext->GetAllocator(), staging.allocation, &data);

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	renderer->ImmediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{ 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
	});

	vmaUnmapMemory(m_pContext->GetAllocator(), staging.allocation);

	//vmaDestroyBuffer(m_pContext->GetAllocator(), staging.buffer, staging.allocation);
	Buffer::DestroyBuffer(m_pContext->GetAllocator(), staging, false, "staging");

	return newSurface;
}

//void AssetManager::SetSurface(std::string name, std::vector<GeoSurface>& surface)
//{
//	m_meshes[name].surfaces = surface;
//}

std::shared_ptr<GLTF::Model> AssetManager::GetModelByName(std::string_view name)
{
	auto it = m_gltfModels.find(std::string(name));
	if (it != m_gltfModels.end() && it->second.has_value()) {
		return it->second.value();
	}
	
	return nullptr;
}

void AssetManager::LoadGltfModel(std::string_view name, std::string_view filePath, ShadingType type)
{
	m_loader.Load(m_pContext, this, m_pRenderer, filePath, std::string(name), type);
}

AllocatedImage AssetManager::CreateImageData(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	AllocatedImage newImage;
	newImage.imageFormat = format;
	newImage.imageExtent = size;

	VkImageCreateInfo img_info = vkinit::CreateImageCreateInfo(format, usage, size);
	if (mipmapped) {
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	VK_CHECK(vmaCreateImage(m_pContext->GetAllocator(), &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image-view for the image
	VkImageViewCreateInfo view_info = vkinit::CreateImageviewCreateInfo(format, newImage.image, aspectFlag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	VK_CHECK(vkCreateImageView(m_pContext->GetDevice(), &view_info, nullptr, &newImage.imageView));

	return newImage;
}

AllocatedImage AssetManager::CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	size_t data_size = size.depth * size.width * size.height * 4;
	assert(m_pContext != nullptr);
	AllocatedBuffer uploadbuffer = Buffer::CreateBuffer(m_pContext->GetAllocator(), data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, false, "image");

	memcpy(uploadbuffer.info.pMappedData, data, data_size);

	AllocatedImage new_image = CreateImageData(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	m_pRenderer->ImmediateSubmit([&](VkCommandBuffer cmd) {
		vkutil::TransitionImage(cmd, new_image.image, new_image.imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = size;

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		if (mipmapped) {
			vkutil::GenerateMipmaps(cmd, new_image.image, new_image.imageFormat, VkExtent2D{ new_image.imageExtent.width, new_image.imageExtent.height });
		}
		else {
			vkutil::TransitionImage(cmd, new_image.image, new_image.imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	});

	//vmaDestroyBuffer(m_pContext->GetAllocator(), uploadbuffer.buffer, uploadbuffer.allocation);
	Buffer::DestroyBuffer(m_pContext->GetAllocator(), uploadbuffer, false , "image");

	//std::cout << "[CREATE IMAGE] " << new_image.image << std::endl;

	return new_image;
}

void AssetManager::DestroyImage(const AllocatedImage& img)
{
	//std::cout << "[DESTROY IMAGE] " << img.image << std::endl;

	vkDestroyImageView(m_pContext->GetDevice(), img.imageView, nullptr);
	vmaDestroyImage(m_pContext->GetAllocator(), img.image, img.allocation);
}

void AssetManager::InitDefaultData()
{
	//std::array<Vertex, 4> rect_vertices;

	//rect_vertices[0].position = { 0.5,-0.5, 0 };
	//rect_vertices[1].position = { 0.5,0.5, 0 };
	//rect_vertices[2].position = { -0.5,-0.5, 0 };
	//rect_vertices[3].position = { -0.5,0.5, 0 };

	//rect_vertices[0].color = { 0,0, 0,1 };
	//rect_vertices[1].color = { 0.5,0.5,0.5 ,1 };
	//rect_vertices[2].color = { 1,0, 0,1 };
	//rect_vertices[3].color = { 0,1, 0,1 };

	//std::array<uint32_t, 6> rect_indices;

	//rect_indices[0] = 0;
	//rect_indices[1] = 1;
	//rect_indices[2] = 2;

	//rect_indices[3] = 2;
	//rect_indices[4] = 1;
	//rect_indices[5] = 3;

	//UploadMesh(m_pRenderer, "rectangle", rect_indices, rect_vertices);

	//3 default textures, white, grey, black. 1 pixel each
	uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	m_whiteImage = CreateImage((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
	m_greyImage = CreateImage((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	m_blackImage = CreateImage((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	//checkerboard image
	uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16 * 16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	m_errorCheckerboardImage = CreateImage(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;

	vkCreateSampler(m_pContext->GetDevice(), &sampl, nullptr, &m_defaultSamplerNearest);

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(m_pContext->GetDevice(), &sampl, nullptr, &m_defaultSamplerLinear);

	m_deletionQueue.PushFunction([&]() {
		vkDestroySampler(m_pContext->GetDevice(), m_defaultSamplerLinear, nullptr);
		vkDestroySampler(m_pContext->GetDevice(), m_defaultSamplerNearest, nullptr);

		DestroyImage(m_whiteImage);
		DestroyImage(m_greyImage);
		DestroyImage(m_blackImage);
		DestroyImage(m_errorCheckerboardImage);
		});
}

void vkutil::TransitionImage(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout currentLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	imageBarrier.pNext = nullptr;

	imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
	imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

	imageBarrier.oldLayout = currentLayout;
	imageBarrier.newLayout = newLayout;

	VkImageAspectFlags aspectMask;
	if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM) {
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else {
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	imageBarrier.subresourceRange = vkinit::ImageSubResourceRange(aspectMask);
	imageBarrier.image = image;

	VkDependencyInfo depInfo{};
	depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	depInfo.pNext = nullptr;

	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imageBarrier;

	vkCmdPipelineBarrier2(cmd, &depInfo);
}

void AssetManager::SetGltfModel(std::string_view name, std::optional<std::shared_ptr<GLTF::Model>> model)
{
	m_gltfModels[std::string(name)] = model;

	//std::cout << "LoadingTest: " << m_gltfModels[std::string(name)].has_value() << std::endl;
}

void vkutil::CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
{
	VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	vkCmdBlitImage2(cmd, &blitInfo);
}

void vkutil::GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkFormat format, VkExtent2D imageSize)
{
	int mipLevels = int(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;
	for (int mip = 0; mip < mipLevels; mip++) {

		VkExtent2D halfSize = imageSize;
		halfSize.width /= 2;
		halfSize.height /= 2;

		VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr };

		imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange = vkinit::ImageSubResourceRange(aspectMask);
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseMipLevel = mip;
		imageBarrier.image = image;

		VkDependencyInfo depInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .pNext = nullptr };
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(cmd, &depInfo);

		if (mip < mipLevels - 1) {
			VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

			blitRegion.srcOffsets[1].x = imageSize.width;
			blitRegion.srcOffsets[1].y = imageSize.height;
			blitRegion.srcOffsets[1].z = 1;

			blitRegion.dstOffsets[1].x = halfSize.width;
			blitRegion.dstOffsets[1].y = halfSize.height;
			blitRegion.dstOffsets[1].z = 1;

			blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.srcSubresource.baseArrayLayer = 0;
			blitRegion.srcSubresource.layerCount = 1;
			blitRegion.srcSubresource.mipLevel = mip;

			blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.dstSubresource.baseArrayLayer = 0;
			blitRegion.dstSubresource.layerCount = 1;
			blitRegion.dstSubresource.mipLevel = mip + 1;

			VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
			blitInfo.dstImage = image;
			blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			blitInfo.srcImage = image;
			blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			blitInfo.filter = VK_FILTER_LINEAR;
			blitInfo.regionCount = 1;
			blitInfo.pRegions = &blitRegion;

			vkCmdBlitImage2(cmd, &blitInfo);

			imageSize = halfSize;
		}
	}

	// transition all mip levels into the final read_only layout
	TransitionImage(cmd, image, format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void DirectionalShadowResources::Init(VulkanContext* context, Renderer* renderer)
{
	//
	// 1. Create shadow images
	//
	shadowImage.imageFormat = VK_FORMAT_D32_SFLOAT;
	shadowImage.imageExtent = extent;

	VkImageUsageFlags shadowImageUsages{};
	shadowImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; // use for depth attachment
	shadowImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT; // use as texture

	VkImageCreateInfo imageInfo = vkinit::CreateImageCreateInfo(shadowImage.imageFormat, shadowImageUsages, extent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo allocationInfo = {};
	allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkResult result = vmaCreateImage(
		context->GetAllocator(),
		&imageInfo,
		&allocationInfo,
		&shadowImage.image,
		&shadowImage.allocation,
		nullptr
	);

	if (result != VK_SUCCESS) {
		throw std::runtime_error(
			"Failed to create shadow map image"
		);
	}

	VkImageViewCreateInfo imageViewInfo = vkinit::CreateImageviewCreateInfo(shadowImage.imageFormat, shadowImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);
	VK_CHECK(vkCreateImageView(context->GetDevice(), &imageViewInfo, nullptr, &shadowImage.imageView));

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = 0.f;

	VK_CHECK(vkCreateSampler(context->GetDevice(), &samplerInfo, nullptr, &shadowSampler));

	//
	// 2. Create shadow map pipeline
	//

	VkFormat depthFormat = shadowImage.imageFormat;

	VkPipelineRenderingCreateInfo renderingCreateInfo{};
	renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingCreateInfo.colorAttachmentCount = 0;
	renderingCreateInfo.pColorAttachmentFormats = nullptr;
	renderingCreateInfo.depthAttachmentFormat = depthFormat;
	renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

	VkShaderModule meshFragShader;
	if (!vkutil::LoadShaderModule("res/shaders/ShadowMapping.frag.spv", context->GetDevice(), &meshFragShader)) {
		fmt::println("Error when building the triangle fragment shader module");
	}

	VkShaderModule meshVertexShader;
	if (!vkutil::LoadShaderModule("res/shaders/ShadowMapping.vert.spv", context->GetDevice(), &meshVertexShader)) {
		fmt::println("Error when building the triangle vertex shader module");
	}

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayout layouts[] = { renderer->GetGlobalDescriptorSetLayout() };

	VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::CreatePipelineLayoutCreateInfo();
	mesh_layout_info.setLayoutCount = 1;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(context->GetDevice(), &mesh_layout_info, nullptr, &layout));

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.disable_blending();
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_LESS);
	pipelineBuilder.enable_rasterizer();

	pipelineBuilder.disable_color_attachment();
	pipelineBuilder.set_depth_format(shadowImage.imageFormat);

	pipelineBuilder._pipelineLayout = layout;
	pipeline = pipelineBuilder.BuildPipeline(context->GetDevice());

	vkDestroyShaderModule(context->GetDevice(), meshFragShader, nullptr);
	vkDestroyShaderModule(context->GetDevice(), meshVertexShader, nullptr);

	//
	// 3. create DescriptorSet for shadowmap
	//

	DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	descriptorSetLayout = layoutBuilder.build(context->GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT);

	// we can stimate the descriptors we will need accurately
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

	allocator.Init(context->GetDevice(), 1, sizes);
	descriptorSet = allocator.Allocate(context->GetDevice(), descriptorSetLayout);

	DescriptorWriter writer;
	writer.clear();
	writer.write_image(0, shadowImage.imageView, shadowSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.update_set(context->GetDevice(), descriptorSet);
}

void DirectionalShadowResources::Cleanup(VulkanContext* context)
{
	vkDestroyImageView(context->GetDevice(), shadowImage.imageView, nullptr);
	vmaDestroyImage(context->GetAllocator(), shadowImage.image, shadowImage.allocation);
	vkDestroySampler(context->GetDevice(), shadowSampler, nullptr);

	vkDestroyPipeline(context->GetDevice(), pipeline, nullptr);	
	vkDestroyPipelineLayout(context->GetDevice(), layout, nullptr);

	allocator.DestroyPools(context->GetDevice());
	vkDestroyDescriptorSetLayout(context->GetDevice(), descriptorSetLayout, nullptr);
}