#include "Engine.h"

void ThirdEngine::init()
{
	m_renderWindow.init();
	m_vulkanContext.init(&m_renderWindow);
	m_renderer.Init(&m_vulkanContext, m_renderWindow);
}

void ThirdEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	while (!bQuit) {
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
}

void ThirdEngine::cleanup()
{
	m_renderer.Cleanup();
	m_vulkanContext.cleanup();
	m_renderWindow.cleanup();
}