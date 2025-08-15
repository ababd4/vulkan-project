#include "Engine.h"

void ThirdEngine::init()
{
	m_renderWindow.init();
	m_vulkanContext.Init(&m_renderWindow);
	m_renderer.Init(&m_vulkanContext, m_renderWindow);
}

void ThirdEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	while (!bQuit) {
		// Detect SDL Events
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				bQuit = true;
			}

			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) { // ESCAPE : exit engine
					bQuit = true;
				}
			}
		}
	}

	m_renderer.UpdateScene();
	// m_renderer.Render();
}

void ThirdEngine::cleanup()
{
	m_renderer.Cleanup();
	m_vulkanContext.Cleanup();
	m_renderWindow.cleanup();
}