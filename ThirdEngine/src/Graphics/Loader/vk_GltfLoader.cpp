#include "vk_GltfLoader.h"

#include "fastgltf/parser.hpp"
#include "fastgltf/tools.hpp"

#include <fastgltf/glm_element_traits.hpp>

void GLTF::Loader::Init(VulkanContext* context, MeshManager* meshManager)
{
	m_pContext = context;
	m_pMeshManager = meshManager;
}

void GLTF::Loader::LoadMesh(const std::filesystem::path path)
{
	std::cout << "Loading: " << path << std::endl;

	fastgltf::GltfDataBuffer data;
	data.loadFromFile(path);

	constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers
		| fastgltf::Options::LoadExternalBuffers;

	fastgltf::Asset gltf;
	fastgltf::Parser parser;

	auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
	if (load) {
		gltf = std::move(load.get());
	}
	else {
		std::cout << "Failed to load glTF: {} \n" << fastgltf::to_underlying(load.error()) << std::endl;
	}

	std::vector<std::shared_ptr<MeshAsset>> meshes;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	for (fastgltf::Mesh& mesh : gltf.meshes) {
		// create new mesh
		MeshAsset newMesh;
		newMesh.name = mesh.name;
		std::cout << mesh.name << std::endl;

		// we dont want to merge by error
		vertices.clear();
		indices.clear();

		for (auto&& p : mesh.primitives) {
			GeoSurface newSurface;
			newSurface.startIndex = (uint32_t)indices.size(); // the end is the start index
			newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

			size_t initial_vtx = vertices.size(); // count the number of vertices elements. It will be the offset of base vertex

			// load indexes
			{
				fastgltf::Accessor& indexAccessor = gltf.accessors[p.indicesAccessor.value()];
				indices.reserve(indices.size() + indexAccessor.count);

				fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor,
					[&](std::uint32_t idx) {
						indices.push_back(idx + initial_vtx);
					});
			}

			// load vertex positions
			{
				fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
				vertices.resize(vertices.size() + posAccessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
					[&](glm::vec3 v, size_t index) {
						Vertex newvtx;
						newvtx.position = v;
						newvtx.normal = { 1, 0, 0 };
						newvtx.color = glm::vec4{ 1.f };
						newvtx.uv_x = 0;
						newvtx.uv_y = 0;
						vertices[initial_vtx + index] = newvtx;
					});
			}

			// load vertex normals
			auto normals = p.findAttribute("NORMAL");
			if (normals != p.attributes.end()) {

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
					[&](glm::vec3 v, size_t index) {
						vertices[initial_vtx + index].normal = v;
					});
			}

			// load UVs
			auto uv = p.findAttribute("TEXCOORD_0");
			if (uv != p.attributes.end()) {

				fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
					[&](glm::vec2 v, size_t index) {
						vertices[initial_vtx + index].uv_x = v.x;
						vertices[initial_vtx + index].uv_y = v.y;
					});
			}

			// load vertex colors
			auto colors = p.findAttribute("COLOR_0");
			if (colors != p.attributes.end()) {

				fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
					[&](glm::vec4 v, size_t index) {
						vertices[initial_vtx + index].color = v;
					});
			}
			newMesh.surfaces.push_back(newSurface);
		}

		// display the vertex normals for debug
		constexpr bool OverrideColors = false;
		if (OverrideColors) {
			for (Vertex& vtx : vertices) {
				vtx.color = glm::vec4(vtx.normal, 1.f);
			}
		}

		std::cout << indices.size() << " " << vertices.size() << std::endl;

		m_pMeshManager->UploadMesh(newMesh.name, indices, vertices);
		m_pMeshManager->SetSurface(newMesh.name, newMesh.surfaces);
	}
}