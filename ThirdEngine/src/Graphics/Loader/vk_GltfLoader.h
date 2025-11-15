#pragma once

#include "../vk_Types.h"
#include "../vk_Context.h"
#include "../Mesh/vk_MeshManager.h"

#include <unordered_map>
#include <filesystem>

namespace GLTF
{
	class Loader
	{
	public:
		void Init(VulkanContext* context, MeshManager* meshManager);
		void LoadMesh(const std::filesystem::path path);

	private:
		VulkanContext* m_pContext;
		MeshManager* m_pMeshManager;
	};
}

