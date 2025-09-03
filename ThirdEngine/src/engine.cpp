#include "Engine.h"

#include <chrono>
#include <thread>

void ThirdEngine::Init()
{
	m_renderWindow.Init();
	m_vulkanContext.Init(&m_renderWindow);
	m_pipelineManager.Init(&m_vulkanContext);
	m_bufferManager.Init(&m_vulkanContext);
	m_meshManager.Init(&m_vulkanContext, &m_bufferManager);
	m_renderer.Init(&m_vulkanContext, m_renderWindow, &m_pipelineManager, &m_meshManager);
}

void ThirdEngine::Run()
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

			if (e.type == SDL_WINDOWEVENT) {
				if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					stopRendering = true;
				} 
				if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
					stopRendering = false;
				}
			}
		}

		// don't draw if the window is minimized
		if (stopRendering) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		m_renderer.UpdateScene();
		m_renderer.Render();
	}
}

void ThirdEngine::Cleanup()
{
	m_renderer.Cleanup();
	m_pipelineManager.Cleanup();
	m_bufferManager.Cleanup();
	m_meshManager.Cleanup();
	m_vulkanContext.Cleanup();
	m_renderWindow.Cleanup();
}