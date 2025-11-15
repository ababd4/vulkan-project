#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

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

struct GeoSurface {
	uint32_t startIndex;
	uint32_t count;
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