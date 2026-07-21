#pragma once

#include "../vk_Types.h"
#include "../vk_Context.h"
#include "../Buffer/Buffer.h"
#include "../Loader/GltfLoader.h"
#include "../Model/Model.h"

#include <span>
#include <map>

class Renderer;

class AssetManager
{
public:
	void Init(VulkanContext* context, Renderer* renderer, PipelineManager* pipelineManager);
	void Cleanup();

	// send vertex data to GPU Memory
	void UploadMesh(Renderer* renderer, std::string name, std::span<uint32_t> indices, std::span<Vertex> vertices);
	

	void LoadGltfModel(std::string_view name, std::string_view filePath);
	AllocatedImage CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void DestroyImage(const AllocatedImage& img);

	// Setter
	void SetSurface(std::string name, std::vector<GeoSurface>& surface);
	void SetGltfModel(std::string_view name, std::optional<std::shared_ptr<GLTF::Model>> model);

	// Getter
	std::shared_ptr<GLTF::Model> GetModelByName(std::string_view name);
	MeshAsset GetMeshByName(std::string name) { return m_meshes[name]; }
	AllocatedImage GetWhiteImage() { return m_whiteImage; }
	AllocatedImage GetBlackImage() { return m_blackImage; }
	AllocatedImage GetGreyImage() { return m_greyImage; }
	AllocatedImage GetErrorImage() { return m_errorCheckerboardImage; }
	VkSampler GetDefaultSamplerLinear() { return m_defaultSamplerLinear; }
	VkSampler GetDefaultSamplerNearest() { return m_defaultSamplerNearest; }
	GLTF::MetallicRoughness GetMetallicRoughness() { return m_metallicRoughness; }

private:
	VulkanContext* m_pContext;
	PipelineManager* m_pPipelineManager;
	Renderer* m_pRenderer;

	// models
	std::unordered_map<std::string, MeshAsset> m_meshes;
	std::unordered_map<std::string, std::optional<std::shared_ptr<GLTF::Model>>> m_gltfModels;

	// textures
	AllocatedImage m_whiteImage;
	AllocatedImage m_blackImage;
	AllocatedImage m_greyImage;
	AllocatedImage m_errorCheckerboardImage;

	VkSampler m_defaultSamplerLinear;
	VkSampler m_defaultSamplerNearest;

	// factory
	GLTF::MetallicRoughness m_metallicRoughness;

	// deletion queue
	DeletionQueue m_deletionQueue;

	AllocatedImage CreateImageData(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
	void InitDefaultData();
};

namespace vkutil {
	void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
};

