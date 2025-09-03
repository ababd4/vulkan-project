#pragma once

#include "Graphics/vk_Context.h"
#include "Graphics/vk_Renderer.h"
#include "Graphics/Pipeline/vk_PipelineManager.h"
#include "Graphics/Buffer/vk_BufferManager.h"
#include "Graphics/Mesh/vk_MeshManager.h"
#include "Window/Window.h"

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
	Window m_renderWindow;
};