#include "GltfLoader.h"

#include "../vk_renderer.h"
#include "../Asset/AssetManager.h"
#include <stb_image/stb_image.h>
#include <glm/gtx/transform.hpp>

void GLTF::Loader::Init(VulkanContext* context, AssetManager* assetManager)
{
	m_pContext = context;
	m_pAssetManager = assetManager;
}

/*
void GLTF::Loader::LoadMesh(Renderer* renderer, const std::filesystem::path path)
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
				vtx.color = glm::vec4(vtx.normal, 0.5f);
			}
		}

		std::cout << indices.size() << " " << vertices.size() << std::endl;



		m_pAssetManager->UploadMesh(renderer, newMesh.name, indices, vertices);
		m_pAssetManager->SetSurface(newMesh.name, newMesh.surfaces);
	}
}
*/

void PrintImageChannelStatistics(
	const unsigned char* data,
	int width,
	int height)
{
	uint64_t sum[4]{};

	unsigned char minimum[4]{
		255, 255, 255, 255
	};

	unsigned char maximum[4]{
		0, 0, 0, 0
	};

	const size_t pixelCount =
		static_cast<size_t>(width) *
		static_cast<size_t>(height);

	for (size_t pixel = 0; pixel < pixelCount; ++pixel)
	{
		for (size_t channel = 0; channel < 4; ++channel)
		{
			const unsigned char value =
				data[pixel * 4 + channel];

			minimum[channel] =
				std::min(minimum[channel], value);

			maximum[channel] =
				std::max(maximum[channel], value);

			sum[channel] += value;
		}
	}

	const char* names[] = {
		"R", "G", "B", "A"
	};

	for (size_t channel = 0; channel < 4; ++channel)
	{
		const double average =
			static_cast<double>(sum[channel]) /
			static_cast<double>(pixelCount);

		std::cout
			<< names[channel]
			<< " min=" << static_cast<int>(minimum[channel])
			<< " max=" << static_cast<int>(maximum[channel])
			<< " avg=" << average
			<< '\n';
	}
}

void GLTF::Loader::Load(VulkanContext* context, AssetManager* assetManager, Renderer* renderer, std::string_view filePath, std::string_view name, ShadingType shadingType)
{
	fmt::print("Loading GLTF: {}\n", filePath);

	std::shared_ptr<GLTF::Model> scene = std::make_shared<GLTF::Model>(context, assetManager);
	GLTF::Model& file = *scene.get();

	fastgltf::Parser parser{};

	constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
	// fastgltf::Options::LoadExternalImages;

	fastgltf::GltfDataBuffer data;
	data.loadFromFile(filePath);

	fastgltf::Asset gltf;

	std::filesystem::path path = filePath;

	auto type = fastgltf::determineGltfFileType(&data);
	if (type == fastgltf::GltfType::glTF) {
		auto load = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
		if (load) {
			gltf = std::move(load.get());
		}
		else {
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return;
		}
	}
	else if (type == fastgltf::GltfType::GLB) {
		auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
		if (load) {
			gltf = std::move(load.get());
		}
		else {
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return;
		}
	}
	else {
		std::cerr << "Failed to determine glTF container" << std::endl;
		return;
	}

	// we can stimate the descriptors we will need accurately
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

	// at least, take care 1 descriptor pool for no material glb model
	uint32_t materialCount = std::max(1u, (uint32_t)gltf.materials.size());

	file.descriptorPool.Init(context->GetDevice(), materialCount, sizes);

	// load samplers
	for (fastgltf::Sampler& sampler : gltf.samplers) {

		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr };
		sampl.maxLod = VK_LOD_CLAMP_NONE;
		sampl.minLod = 0;	

		sampl.magFilter = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		sampl.minFilter = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		sampl.mipmapMode = ExtractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		VkSampler newSampler;
		vkCreateSampler(context->GetDevice(), &sampl, nullptr, &newSampler);

		file.samplers.push_back(newSampler);
	}

	// temporal arrays for all the objects to use while creating the GLTF data
	std::vector<std::shared_ptr<MeshAsset>> meshes;
	std::vector<std::shared_ptr<Node>> nodes;
	std::vector<AllocatedImage> images;
	std::vector<std::shared_ptr<Material>> materials;

	// load all textures
	for (fastgltf::Image& image : gltf.images) {
		std::optional<AllocatedImage> img = LoadImage(assetManager, gltf, image, filePath);

		if (img.has_value()) {
			images.push_back(*img);
			file.images.push_back(*img);
		}
		else {
			// we failed to load, so lets give the slot a default white texture to not
			// completely break loading
			images.push_back(assetManager->GetErrorImage());
			std::cout << "gltf failed to load texture " << image.name << std::endl;
		}
	}

	// create buffer to hold the material data
	file.materialDataBuffer = Buffer::CreateBuffer(context->GetAllocator(), sizeof(GLTF::MaterialSystem::MaterialConstants) * materialCount,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, false, "materialData");
	int data_index = 0;
	GLTF::MaterialSystem::MaterialConstants* sceneMaterialConstants = (GLTF::MaterialSystem::MaterialConstants*)file.materialDataBuffer.info.pMappedData;

	if (gltf.materials.empty())
	{
		std::shared_ptr<Material> defaultMat =
			std::make_shared<Material>();

		GLTF::MaterialSystem::MaterialResources resources;

		resources.colorImage = assetManager->GetGreyImage();
		resources.colorSampler = assetManager->GetDefaultSamplerLinear();

		resources.metalRoughImage = assetManager->GetWhiteImage();
		resources.metalRoughSampler = assetManager->GetDefaultSamplerLinear();
			
		resources.dataBuffer = file.materialDataBuffer.buffer;
		resources.dataBufferOffset = 0;

		defaultMat->data =
			assetManager->GetMaterialSystem(shadingType).WriteMaterial(
				context->GetDevice(),
				MaterialPass::MainColor,
				resources,
				file.descriptorPool);

		materials.push_back(defaultMat);

		file.materials["_default"] = defaultMat;
	}

	for (fastgltf::Material& mat : gltf.materials) {
		std::shared_ptr<Material> newMat = std::make_shared<Material>();
		materials.push_back(newMat);
		file.materials[mat.name.c_str()] = newMat;

		GLTF::MaterialSystem::MaterialConstants constants{};
		constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
		constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
		constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
		constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

		constants.metalRoughFactors.x = mat.pbrData.metallicFactor;
		constants.metalRoughFactors.y = mat.pbrData.roughnessFactor;

		// set default value
		constants.extraData.x = 0.5f;
		constants.extraData.x = static_cast<float>(mat.alphaCutoff);
		switch (mat.alphaMode) 
		{
		case fastgltf::AlphaMode::Opaque:
			constants.extraData.y = static_cast<float>(GPUAlphaMode::Opaque);
			break;
		case fastgltf::AlphaMode::Mask:
			constants.extraData.y = static_cast<float>(GPUAlphaMode::Mask);
			break;
		case fastgltf::AlphaMode::Blend:
			constants.extraData.y = static_cast<float>(GPUAlphaMode::Blend);
			break;
		}

		// write material parameters to buffer
		sceneMaterialConstants[data_index] = constants;

		MaterialPass passType = MaterialPass::MainColor;
		if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
			passType = MaterialPass::Transparent;
		}

		GLTF::MaterialSystem::MaterialResources materialResources;
		// default the material textures
		materialResources.colorImage = assetManager->GetWhiteImage();
		materialResources.colorSampler = assetManager->GetDefaultSamplerLinear();
		materialResources.metalRoughImage = assetManager->GetWhiteImage();
		materialResources.metalRoughSampler = assetManager->GetDefaultSamplerLinear();
		materialResources.normalImage = assetManager->GetWhiteImage();
		materialResources.normalSampler = assetManager->GetDefaultSamplerLinear();

		// set the uniform buffer for the material data
		materialResources.dataBuffer = file.materialDataBuffer.buffer;
		materialResources.dataBufferOffset = data_index * sizeof(GLTF::MaterialSystem::MaterialConstants);
		// grab textures from gltf file
		// color textures
		if (mat.pbrData.baseColorTexture.has_value()) {
			const auto& textureInfo = mat.pbrData.baseColorTexture.value();
			const size_t textureIndex = textureInfo.textureIndex;

			if (textureIndex < gltf.textures.size()) {
				const fastgltf::Texture& texture = gltf.textures[textureIndex];

				if (texture.imageIndex.has_value()) {
					const size_t imageIndex = texture.imageIndex.value();

					if (imageIndex < images.size()) {
						materialResources.colorImage = images[imageIndex];
					}
				}

				if (texture.samplerIndex.has_value()) {
					const size_t samplerIndex = texture.samplerIndex.value();

					if (samplerIndex < file.samplers.size()) {
						materialResources.colorSampler = file.samplers[samplerIndex];
					}
				}
			}
		}

		// metallic roughness texture
		if (mat.pbrData.metallicRoughnessTexture.has_value()) {
			const auto& textureInfo = mat.pbrData.metallicRoughnessTexture.value();
			const size_t textureIndex = textureInfo.textureIndex;

			if (textureIndex < gltf.textures.size()) {
				const fastgltf::Texture& texture = gltf.textures[textureIndex];

				if (texture.imageIndex.has_value()) {
					const size_t imageIndex = texture.imageIndex.value();

					if (imageIndex < images.size()) {
						materialResources.metalRoughImage = images[imageIndex];
					}
				}

				if (texture.samplerIndex.has_value()) {
					const size_t samplerIndex = texture.samplerIndex.value();

					if (samplerIndex < file.samplers.size()) {
						materialResources.metalRoughSampler = file.samplers[samplerIndex];
					}
				}
			}
		}

		// normal textures
		if (mat.normalTexture.has_value()) {
			size_t img = gltf.textures[mat.normalTexture.value().textureIndex].imageIndex.value();
			size_t sampler = gltf.textures[mat.normalTexture.value().textureIndex].samplerIndex.value();

			materialResources.normalImage = images[img];
			materialResources.normalSampler = file.samplers[sampler];
		}

		// build material
		newMat->data = assetManager->GetMaterialSystem(shadingType).WriteMaterial(context->GetDevice(), passType, materialResources, file.descriptorPool);

		data_index++;
	}

	// use the same vectors for all meshes so that the memory doesnt reallocate as often
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;

	for (fastgltf::Mesh& mesh : gltf.meshes) {
		std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
		meshes.push_back(newmesh);
		file.meshes[mesh.name.c_str()] = newmesh;
		newmesh->name = mesh.name;

		// clear the mesh arrays each mesh
		indices.clear();
		vertices.clear();

		for (auto&& p : mesh.primitives) {
			GeoSurface newSurface;
			newSurface.startIndex = (uint32_t)indices.size();
			newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

			size_t initial_vtx = vertices.size();

			// load indexes
			{
				fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
				indices.reserve(indices.size() + indexaccessor.count);

				fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
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

			// load tangent
			auto tangent = p.findAttribute("TANGENT");
			if (tangent != p.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*tangent).second],
					[&](glm::vec4 v, size_t index) {
						vertices[initial_vtx + index].tangent = v;
					});
			}

			if (p.materialIndex.has_value() && p.materialIndex.value() < materials.size()) {
				newSurface.material = materials[p.materialIndex.value()];
			}
			else {
				newSurface.material = materials[0];
			}

			//loop the vertices of this surface, find min/max bounds
			glm::vec3 minpos = vertices[initial_vtx].position;
			glm::vec3 maxpos = vertices[initial_vtx].position;
			for (int i = initial_vtx; i < vertices.size(); i++) {
				minpos = glm::min(minpos, vertices[i].position);
				maxpos = glm::max(maxpos, vertices[i].position);
			}
			// calculate origin and extents from the min/max, use extent lenght for radius
			newSurface.bounds.origin = (maxpos + minpos) / 2.f;
			newSurface.bounds.extents = (maxpos - minpos) / 2.f;
			newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);

			newmesh->surfaces.push_back(newSurface);
		}

		newmesh->meshBuffers = assetManager->UploadMesh(renderer, std::string(name), indices, vertices);
	}


	// load all nodes and their meshes
	for (fastgltf::Node& node : gltf.nodes) {
		std::shared_ptr<Node> newNode;

		// find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
		if (node.meshIndex.has_value()) {
			newNode = std::make_shared<MeshNode>();
			static_cast<MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
		}
		else {
			newNode = std::make_shared<Node>();
		}

		nodes.push_back(newNode);
		file.nodes[node.name.c_str()] = newNode;

		std::visit(fastgltf::visitor{ [&](fastgltf::Node::TransformMatrix matrix) {
										  memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
									  },
					   [&](fastgltf::Node::TRS transform) {
						   glm::vec3 tl(transform.translation[0], transform.translation[1],
							   transform.translation[2]);
						   glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1],
							   transform.rotation[2]);
						   glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

						   glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
						   glm::mat4 rm = glm::toMat4(rot);
						   glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

						   newNode->localTransform = tm * rm * sm;
					   } },
			node.transform);
	}

	// run loop again to setup transform hierarchy
    for (int i = 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node& node = gltf.nodes[i];
        std::shared_ptr<Node>& sceneNode = nodes[i];

        for (auto& c : node.children) {
            sceneNode->children.push_back(nodes[c]);
            nodes[c]->parent = sceneNode;
        }
    }

    // find the top nodes, with no parents
    for (auto& node : nodes) {
        if (node->parent.lock() == nullptr) {
            file.topNodes.push_back(node);
            node->refreshTransform(glm::mat4 { 1.f });
        }
    }
    
	//return scene;
	assetManager->SetGltfModel(std::string(name), scene);

	std::cout << "Successfully Loaded: " << std::string(name) << std::endl;

	for (auto& mesh : gltf.meshes)
	{
		std::cout << mesh.name
			<< " primitives="
			<< mesh.primitives.size()
			<< std::endl;
	}
}

// gltf using the numbers and properties from OpenGL, which do not match the vulkan ones, we need some conversion function
VkFilter GLTF::Loader::ExtractFilter(fastgltf::Filter filter)
{
	switch (filter) {
		// nearest samplers
	case fastgltf::Filter::Nearest:
	case fastgltf::Filter::NearestMipMapNearest:
	case fastgltf::Filter::NearestMipMapLinear:
		return VK_FILTER_NEAREST;

		// linear samplers
	case fastgltf::Filter::Linear:
	case fastgltf::Filter::LinearMipMapNearest:
	case fastgltf::Filter::LinearMipMapLinear:
	default:
		return VK_FILTER_LINEAR;
	}
}

VkSamplerMipmapMode GLTF::Loader::ExtractMipmapMode(fastgltf::Filter filter)
{
	switch (filter) {
	case fastgltf::Filter::NearestMipMapNearest:
	case fastgltf::Filter::LinearMipMapNearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;

	case fastgltf::Filter::NearestMipMapLinear:
	case fastgltf::Filter::LinearMipMapLinear:
	default:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

std::optional<AllocatedImage> GLTF::Loader::LoadImage(AssetManager* assetManager, fastgltf::Asset& asset, fastgltf::Image& image, const std::filesystem::path& gltfPath)
{
	AllocatedImage newImage{};

	int width, height, nrChannels;

	std::visit(
		fastgltf::visitor{
			[](auto& arg) {},
			[&](fastgltf::sources::URI& filePath) {
				assert(filePath.fileByteOffset == 0);
				assert(filePath.uri.isLocalPath());

				const std::string relativePath(filePath.uri.path().begin(), filePath.uri.path().end());
				const std::filesystem::path fullPath = (gltfPath.parent_path() / relativePath).lexically_normal();

				unsigned char* data = stbi_load(fullPath.string().c_str(), &width, &height, &nrChannels, 4);
				if (data) {
					//PrintImageChannelStatistics(data, width, height);

					VkExtent3D imagesize;
					imagesize.width = width;
					imagesize.height = height;
					imagesize.depth = 1;

					newImage = assetManager->CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

					stbi_image_free(data);
				}
			},
			[&](fastgltf::sources::Vector& vector) {
				unsigned char* data = stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()),
					&width, &height, &nrChannels, 4);
				if (data) {
					//PrintImageChannelStatistics(data, width, height);

					VkExtent3D imagesize;
					imagesize.width = width;
					imagesize.height = height;
					imagesize.depth = 1;

					newImage = assetManager->CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

					stbi_image_free(data);
				}
			},
			[&](fastgltf::sources::BufferView& view) {
				auto& bufferView = asset.bufferViews[view.bufferViewIndex];
				auto& buffer = asset.buffers[bufferView.bufferIndex];

				std::visit(fastgltf::visitor{ // We only care about VectorWithMime here, because we
											  // specify LoadExternalBuffers, meaning all buffers
											  // are already loaded into a vector.
					[](auto& arg) {},
					[&](fastgltf::sources::Vector& vector) {
						unsigned char* data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset,
							static_cast<int>(bufferView.byteLength),
							&width, &height, &nrChannels, 4);
						if (data) {
							//PrintImageChannelStatistics(data, width, height);

							VkExtent3D imagesize;
							imagesize.width = width;
							imagesize.height = height;
							imagesize.depth = 1;

							newImage = assetManager->CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

							stbi_image_free(data);
						}
					} },
					buffer.data);
			},
		},
		image.data);

	// if any of the attempts to load the data failed, we havent written the image
	// so handle is null
	if (newImage.image == VK_NULL_HANDLE) {
		return {};
	}
	else {
		return newImage;
	}
}

