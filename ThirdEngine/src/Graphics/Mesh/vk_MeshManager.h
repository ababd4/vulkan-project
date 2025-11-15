#pragma once

#include "../vk_Types.h"
#include "../vk_Context.h"
#include "../Buffer/vk_BufferManager.h"

#include <span>
#include <map>

class MeshManager
{
public:
	void Init(VulkanContext* context, BufferManager* bufferManager);
	void Cleanup();

	// send vertex data to GPU Memory
	void UploadMesh(std::string name, std::span<uint32_t> indices, std::span<Vertex> vertices);
	void SetSurface(std::string name, std::vector<GeoSurface>& surface);

	MeshAsset GetMeshByName(std::string name) { return m_meshes[name]; }

private:
	VulkanContext* m_pContext;
	BufferManager* m_pBufferManager;

	std::unordered_map<std::string, MeshAsset> m_meshes;

	void InitDefaultData();
};

