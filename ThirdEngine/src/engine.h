#pragma once

#include "Graphics/vk_Context.h"
#include "Graphics/vk_Renderer.h"
#include "Graphics/Pipeline/vk_PipelineManager.h"
#include "Window/Window.h"

class ThirdEngine
{
public:

	void init();
	void run();
	void cleanup();

private:

	bool stopRendering = false;

	VulkanContext m_vulkanContext;
	PipelineManager m_pipelineManager;
	Renderer m_renderer;
	Window m_renderWindow;
};