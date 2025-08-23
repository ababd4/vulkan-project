#pragma once

#include "SDL/SDL.h"
#include "SDL/SDL_vulkan.h"

#include "../Graphics/vk_Types.h"

class Window {
public:
	void Init();
	void Cleanup();

	SDL_Window* GetWindow() { return m_window; }
	VkExtent2D GetWindowExtent() { return m_windowExtent; }

private:
	struct SDL_Window* m_window{ nullptr };

	VkExtent2D m_windowExtent{ 800, 600 };
};