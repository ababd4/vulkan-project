#pragma once

#include "../vk_Types.h"
#include "../vk_Descriptors.h"
#include "../Pipeline/vk_PipelineManager.h"

struct DrawContext {
	std::vector<RenderObject> OpaqueSurfaces;
};

class AssetManager;

namespace GLTF {
	struct Model : public IRenderable
	{
		// storage for all the data on a given glTF file
		std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
		std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
		std::unordered_map<std::string, AllocatedImage> images;
		std::unordered_map<std::string, std::shared_ptr<Material>> materials;

		// nodes that dont have a parent, for iterating through the file in tree order
		std::vector<std::shared_ptr<Node>> topNodes;

		std::vector<VkSampler> samplers;

		DescriptorAllocatorGrowable descriptorPool;

		AllocatedBuffer materialDataBuffer{};

		Model(VulkanContext* context, AssetManager* assetManager) : m_pContext(context), m_pAssetManager(assetManager) {}
		~Model() { ClearAll(); };

		virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

	private:

		void ClearAll();
		VulkanContext* m_pContext = nullptr;
		AssetManager* m_pAssetManager = nullptr;
	};

	struct MeshNode : public Node {

		std::shared_ptr<MeshAsset> mesh;

		virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
	};


	// all you need for objects that express metallic and roughness
	struct MetallicRoughness {
		MaterialPipeline opaquePipeline;
		MaterialPipeline transparentPipeline;

		VkDescriptorSetLayout materialLayout;

		struct MaterialConstants {
			glm::vec4 colorFactors;
			glm::vec4 metal_rough_factors;
			//padding, we need it anyway for uniform buffers
			glm::vec4 extra[14];
		};

		struct MaterialResources {
			AllocatedImage colorImage;
			VkSampler colorSampler;
			AllocatedImage metalRoughImage;
			VkSampler metalRoughSampler;
			VkBuffer dataBuffer;
			uint32_t dataBufferOffset;
		};

		DescriptorWriter writer;

		void ClearResources(VkDevice device);

		MaterialInstance WriteMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
	};
};