#pragma once

#include "../Types.h"
#include "../VkContext.h"
#include "../Model/Model.h"

#include "fastgltf/parser.hpp"
#include "fastgltf/tools.hpp"
#include <fastgltf/glm_element_traits.hpp>

#include <unordered_map>
#include <filesystem>

class Renderer;
class AssetManager;

namespace GLTF
{
	struct Loader
	{
	public:
		void Init(VulkanContext* context, AssetManager* AssetManager);
		//void LoadMesh(Renderer* renderer, const std::filesystem::path path);
		void Load(VulkanContext* context, AssetManager* assetManager, Renderer* renderer, std::string_view filePath, std::string_view name, ShadingType type);
		VkFilter ExtractFilter(fastgltf::Filter filter);
		VkSamplerMipmapMode ExtractMipmapMode(fastgltf::Filter filter);


	private:
		VulkanContext* m_pContext;
		AssetManager* m_pAssetManager;

		std::optional<AllocatedImage> LoadImage(AssetManager* assetManager, fastgltf::Asset& asset, fastgltf::Image& image, const std::filesystem::path& gltfPath);
	};
}


