#include "../Model/Model.h"
#include "../Buffer/Buffer.h"
#include "../Asset/AssetManager.h"

void GLTF::Model::ClearAll()
{
	descriptorPool.DestroyPools(m_pContext->GetDevice());
	vmaDestroyBuffer(m_pContext->GetAllocator(), materialDataBuffer.buffer, materialDataBuffer.allocation);

	for (auto& [k, v] : meshes)
	{
		vmaDestroyBuffer(m_pContext->GetAllocator(), v->meshBuffers.indexBuffer.buffer, v->meshBuffers.indexBuffer.allocation);
		vmaDestroyBuffer(m_pContext->GetAllocator(), v->meshBuffers.vertexBuffer.buffer, v->meshBuffers.vertexBuffer.allocation);

		//creator->destroy_buffer(v->meshBuffers.indexBuffer);
		//creator->destroy_buffer(v->meshBuffers.vertexBuffer);
	}

	for (auto& [k, v] : images) {

		if (v.image == m_pAssetManager->GetErrorImage().image) {
			//dont destroy the default images
			continue;
		}
		m_pAssetManager->DestroyImage(v);
	}

	for (auto& sampler : samplers) {
		vkDestroySampler(m_pContext->GetDevice(), sampler, nullptr);
	}
}

void GLTF::MetallicRoughness::ClearResources(VkDevice device)
{

}

MaterialInstance GLTF::MetallicRoughness::WriteMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
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

		def.transform = nodeMatrix;
		def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

		ctx.OpaqueSurfaces.push_back(def);
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