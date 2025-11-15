#pragma once

#include "Graphics/vk_Context.h"
#include "Graphics/vk_Renderer.h"
#include "Graphics/Pipeline/vk_PipelineManager.h"
#include "Graphics/Buffer/vk_BufferManager.h"
#include "Graphics/Mesh/vk_MeshManager.h"
#include "Graphics/Scene/vk_Scene.h"
#include "Camera/camera.h"
#include "Window/Window.h"
#include "Graphics/Loader/vk_GltfLoader.h"

class ThirdEngine
{
public:

	void Init();
	void Run();
	void Cleanup();

private:

	bool stopRendering = false;

	VulkanContext m_vulkanContext;
	PipelineManager m_pipelineManager;
	BufferManager m_bufferManager;
	MeshManager m_meshManager;
	Renderer m_renderer;
	Scene m_scene;
	Window m_renderWindow;
	GLTF::Loader m_GltfLoader;

	void LoadModels();
};