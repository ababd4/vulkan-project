#pragma once

#include "Graphics/vk_context.h"
#include "Graphics/vk_renderer.h"
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