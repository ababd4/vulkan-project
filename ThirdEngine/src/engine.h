#pragma once

#include "Graphics/vk_context.h"
#include "Graphics/vk_renderer.h"

class ThirdEngine
{
public:

	void init();
	void run();
	void cleanup();

private:

	VulkanContext _vulkanContext;
	Renderer _renderer;
};