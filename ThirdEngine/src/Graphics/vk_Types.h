#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <deque>
#include <functional>
#include <fmt/base.h>

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void PushFunction(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void Flush()
	{
		// reverse itrate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); // call functors
		}

		deletors.clear();
	}

	void PrintContents() {
		fmt::print("Deletion Queue Contents:\n");
		fmt::print("Total number of deletion functions: {}\n", deletors.size());

		for (size_t i = 0; i < deletors.size(); ++i) {
			fmt::print("Deletion Function [{}]: {}\n",
				i,
				typeid(deletors[i]).name()
			);
		}
	}
};

struct AllocatedBuffer {
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo info;
};

struct GPUDrawPushConstants {
	// model matrix
	glm::mat4 worldMatrix;

	// GPU buffer's device address
	// Used in shaders to access vertex data directly
	VkDeviceAddress vertexBuffer;
};

struct AllocatedImage {
	VkImage image;
	VkImageView imageView;
	VkFormat imageFormat;
	VkExtent3D imageExtent;
	VmaAllocation allocation;
};

struct Vertex {
	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
};

struct GPUSceneData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

struct Transform {
	glm::vec3 position{ 0 };
	glm::quat rotation{ 1.f, 0.f, 0.f, 0.f };
	glm::vec3 scale{ 1.f, 1.f, 1.f };
};

struct Entity {
	Transform transform;
	uint32_t meshID;
	uint32_t materialID;
};

// hold the resources for mesh drawing
struct GPUMeshBuffers {
	AllocatedBuffer indexBuffer;
	AllocatedBuffer vertexBuffer;
	VkDeviceAddress vertexBufferAddress;
};

enum class MaterialPass :uint8_t {
	MainColor,
	Transparent,
	Other
};

struct MaterialPipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct MaterialInstance {
	MaterialPipeline* pipeline;
	VkDescriptorSet materialSet;
	MaterialPass passType;
};

struct RenderObject {
	uint32_t indexCount;
	uint32_t firstIndex;
	VkBuffer indexBuffer;

	MaterialInstance* material;

	glm::mat4 transform;
	VkDeviceAddress vertexBufferAddress;
};

namespace GLTF {
	struct Material {
		MaterialInstance data;
	};
};

struct GeoSurface {
	uint32_t startIndex;
	uint32_t count;
	std::shared_ptr<GLTF::Material> material;
};

struct MeshAsset {
	std::string name;

	std::vector<GeoSurface> surfaces;
	GPUMeshBuffers meshBuffers;
};

struct PipelineDesc {
	std::string vert;
	std::string frag;	

	VkRenderPass renderPass;
	uint32_t subpass;

	// compare hash
	bool operator==(const PipelineDesc& o) const noexcept { // const: This member function doesnt change member variables
		return												// noexept: In compare function, there is no exeption.
			vert == o.vert &&
			frag == o.frag &&
			renderPass == o.renderPass &&
			subpass == o.subpass;
	}
};

// hash function for pipeline description
struct PipelineDescHash {
	size_t operator()(const PipelineDesc& d) const noexcept {
		size_t h = 0;
		auto hash_combine = [&](size_t v) {
			h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
			};

		hash_combine(std::hash<std::string>{}(d.vert));
		hash_combine(std::hash<std::string>{}(d.frag));
		hash_combine(std::hash<uint64_t>{}(reinterpret_cast<uint64_t>(d.renderPass)));
		hash_combine(std::hash<uint32_t>{}(d.subpass));
		return h;
	}
};

struct DrawContext;

// base class for a renderable dynamic object
class IRenderable {

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

// implementation of a drawable scene node.
// the scene node can hold children and will also keep a transform to propagate
// to them
struct Node : public IRenderable {

	// parent pointer must be a weak pointer to avoid circular dependencies
	std::weak_ptr<Node> parent;
	std::vector<std::shared_ptr<Node>> children;

	glm::mat4 localTransform;
	glm::mat4 worldTransform;

	void refreshTransform(const glm::mat4& parentMatrix)
	{
		worldTransform = parentMatrix * localTransform;
		for (auto c : children) {
			c->refreshTransform(worldTransform);
		}
	}

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx)
	{
		// draw children
		for (auto& c : children) {
			c->Draw(topMatrix, ctx);
		}
	}
};


