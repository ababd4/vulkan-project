#include "vk_MeshManager.h"

#include "../../Util/Util.h"

#include <array>

void MeshManager::Init(VulkanContext* context, BufferManager* bufferManager)
{
	m_pContext = context;
	m_pBufferManager = bufferManager;
	InitDefaultData();
}

void MeshManager::Cleanup()
{
	for (auto& [name, mesh] : m_meshes) {
		vmaDestroyBuffer(m_pContext->GetAllocator(), mesh.meshBuffers.vertexBuffer.buffer, mesh.meshBuffers.vertexBuffer.allocation);
		vmaDestroyBuffer(m_pContext->GetAllocator(), mesh.meshBuffers.indexBuffer.buffer, mesh.meshBuffers.indexBuffer.allocation);
	}
	m_meshes.clear();
}

void MeshManager::UploadMesh(std::string name, std::span<uint32_t> indices, std::span<Vertex> vertices)
{
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	GPUMeshBuffers newSurface;

	//create vertex buffer
	newSurface.vertexBuffer = m_pBufferManager->CreateBuffer(
		vertexBufferSize, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY
	);

	//find the adress of the vertex buffer
	VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
	newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(m_pContext->GetDevice(), &deviceAdressInfo);

	//create index buffer
	newSurface.indexBuffer = m_pBufferManager->CreateBuffer(
		indexBufferSize, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY
	);

	AllocatedBuffer staging = m_pBufferManager->CreateStagingBuffer(vertexBufferSize + indexBufferSize);

	void* data;
	vmaMapMemory(m_pContext->GetAllocator(), staging.allocation, &data);

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	m_pBufferManager->ImmediateSubmit([&](VkCommandBuffer cmd) {
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

	m_pBufferManager->DestroyBuffer(staging);

	std::cout << "Loaded: " << name << std::endl;
	m_meshes[name].meshBuffers = newSurface;
}

void MeshManager::SetSurface(std::string name, std::vector<GeoSurface>& surface)
{
	m_meshes[name].surfaces = surface;
}

void MeshManager::InitDefaultData() {
	std::array<Vertex, 4> rect_vertices;

	rect_vertices[0].position = { 0.5,-0.5, 0 };
	rect_vertices[1].position = { 0.5,0.5, 0 };
	rect_vertices[2].position = { -0.5,-0.5, 0 };
	rect_vertices[3].position = { -0.5,0.5, 0 };

	rect_vertices[0].color = { 0,0, 0,1 };
	rect_vertices[1].color = { 0.5,0.5,0.5 ,1 };
	rect_vertices[2].color = { 1,0, 0,1 };
	rect_vertices[3].color = { 0,1, 0,1 };

	std::array<uint32_t, 6> rect_indices;

	rect_indices[0] = 0;
	rect_indices[1] = 1;
	rect_indices[2] = 2;

	rect_indices[3] = 2;
	rect_indices[4] = 1;
	rect_indices[5] = 3;

	UploadMesh("rectangle", rect_indices, rect_vertices);
}