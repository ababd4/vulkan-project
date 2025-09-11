#include "vk_GltfLoader.h"

void GLTF::Loader::Init(VulkanContext* context, MeshManager* meshManager)
{
	m_pContext = context;
	m_pMeshManager = meshManager;
}