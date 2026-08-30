#pragma once

#include "../Types.h"
#include "../Descriptors.h"
#include "../Swapchain.h"

struct DrawContext {
	std::vector<RenderObject> OpaqueSurfaces;
	std::vector<RenderObject> TransparentSurfaces;
};

class AssetManager;
class Renderer;

namespace GLTF {
	struct Model : public IRenderable
	{
		// storage for all the data on a given glTF file
		std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
		std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
		//std::unordered_map<std::string, AllocatedImage> images;
		std::vector<AllocatedImage> images;
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

	// base interface for material system
	//class IMaterialSystem {
	//public:
	//	virtual ~IMaterialSystem() = default;

	//	virtual MaterialInstance WriteMaterial(

	//	)
	//};

	// all you need for objects that express metallic and roughness
	struct MaterialSystem {
		MaterialPipeline opaquePipeline;
		MaterialPipeline transparentPipeline;

		VkDescriptorSetLayout materialLayout;

		struct MaterialConstants {
			//
			// 0: red
			// 1: green
			// 2: blue
			// 3: alpha
			//
			glm::vec4 colorFactors;

			//
			// 0: metallic factor
			// 1: roughness factor
			// 2: null
			// 3: null
			//
			glm::vec4 metalRoughFactors;

			//
			// 0: alphaCutoff
			// 1: alphaMode
			// 2: normalScale
			// 3: occulusionStrength
			//
			glm::vec4 extraData;

			//padding, we need it anyway for uniform buffers
			glm::vec4 extra[13];
		};

		struct MaterialResources {
			AllocatedImage colorImage;
			VkSampler colorSampler;
			AllocatedImage metalRoughImage;
			VkSampler metalRoughSampler;
			AllocatedImage normalImage;
			VkSampler normalSampler;
			VkBuffer dataBuffer;
			uint32_t dataBufferOffset;
		};

		DescriptorWriter writer;

		void Init(VkDevice device, Swapchain swapchain, Renderer* renderer, std::string vert, std::string frag);
		void ClearResources(VkDevice device);

		MaterialInstance WriteMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
	};
};