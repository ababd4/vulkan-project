#pragma once

#include "SDL/SDL.h"
#include "SDL/SDL_vulkan.h"

#include "../Util/types.h"

class Window {
public:
	void init();
	void cleanup();

	SDL_Window* GetWindow() { return m_window; }
	VkExtent2D GetWindowExtent() { return m_windowExtent; }

private:
	struct SDL_Window* m_window{ nullptr };

	VkExtent2D m_windowExtent{ 800, 600 };
};