#pragma once

#include "Graphics/vk_Context.h"
#include "Graphics/vk_Renderer.h"
#include "Window/Window.h"

class ThirdEngine
{
public:

	void init();
	void run();
	void cleanup();

private:

	VulkanContext m_vulkanContext;
	Renderer m_renderer;
	Window m_renderWindow;
};