#pragma once

#include "../Types.h"
#include "../VkContext.h"
#include "../Buffer/Buffer.h"
#include "../Loader/GltfLoader.h"
#include "../Model/Model.h"

#include <span>
#include <map>

class Renderer;
class Swapchain;

class AssetManager
{
public:
	void Init(VulkanContext* context, Renderer* renderer);
	void Cleanup();

	// send vertex data to GPU Memory
	GPUMeshBuffers UploadMesh(Renderer* renderer, std::string name, std::span<uint32_t> indices, std::span<Vertex> vertices);
	

	void LoadGltfModel(std::string_view name, std::string_view filePath, ShadingType type);
	AllocatedImage CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void DestroyImage(const AllocatedImage& img);

	// Setter
	void SetGltfModel(std::string_view name, std::optional<std::shared_ptr<GLTF::Model>> model);

	// Getter
	std::shared_ptr<GLTF::Model> GetModelByName(std::string_view name);
	AllocatedImage GetWhiteImage() { return m_whiteImage; }
	AllocatedImage GetBlackImage() { return m_blackImage; }
	AllocatedImage GetGreyImage() { return m_greyImage; }
	AllocatedImage GetErrorImage() { return m_errorCheckerboardImage; }
	VkSampler GetDefaultSamplerLinear() { return m_defaultSamplerLinear; }
	VkSampler GetDefaultSamplerNearest() { return m_defaultSamplerNearest; }
	GLTF::MaterialSystem& GetMaterialSystem(ShadingType type) { return m_materialSystems[type]; }

	// Temporary define. should not be here
	void InitMaterials(Swapchain& swapchain);
	void InitDefaultData();
	void LoadModels();

private:
	VulkanContext* m_pContext;
	Renderer* m_pRenderer;

	// loader
	GLTF::Loader m_loader;

	// models
	std::unordered_map<std::string, std::optional<std::shared_ptr<GLTF::Model>>> m_gltfModels;

	// textures
	AllocatedImage m_whiteImage;
	AllocatedImage m_blackImage;
	AllocatedImage m_greyImage;
	AllocatedImage m_errorCheckerboardImage;

	VkSampler m_defaultSamplerLinear;
	VkSampler m_defaultSamplerNearest;

	// HDR Image
	HDRImageData m_hdrImage;

	// factory
	std::unordered_map<ShadingType, GLTF::MaterialSystem> m_materialSystems;

	// deletion queue
	DeletionQueue m_deletionQueue;

	AllocatedImage CreateImageData(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
};

namespace vkutil {
	void TransitionImage(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout currentLayout, VkImageLayout newLayout);
	void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
	void GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkFormat format, VkExtent2D imageSize);
};

struct DirectionalShadowResources {
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout layout = VK_NULL_HANDLE;

	AllocatedImage shadowImage{};
	VkSampler shadowSampler = VK_NULL_HANDLE;

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

	DescriptorAllocatorGrowable allocator;

	VkExtent3D extent = {
		2048,
		2048,
		1
	};

	void Init(VulkanContext* context, Renderer* renderer);
	void Cleanup(VulkanContext* context);
};