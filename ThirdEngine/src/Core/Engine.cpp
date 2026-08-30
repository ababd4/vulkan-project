#include "Engine.h"

#include <chrono>
#include <thread>

void ThirdEngine::Init()
{
	m_renderWindow.Init();
	m_vulkanContext.Init(&m_renderWindow);
	m_AssetManager.Init(&m_vulkanContext, &m_renderer);
	m_renderer.Init(&m_vulkanContext, m_renderWindow, &m_AssetManager, &m_scene, &m_imGuiLayer, &m_stats);
	m_scene.Init(&m_renderWindow, &m_AssetManager);

	m_AssetManager.InitDefaultData();
	m_AssetManager.InitMaterials(m_renderer.m_swapchain);
	m_AssetManager.LoadModels();
}

void ThirdEngine::Run()
{
	SDL_Event e;
	bool bQuit = false;

	while (!bQuit) {
		// begin clock
		auto start = std::chrono::system_clock::now();

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
				} 				if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
					stopRendering = false;
				}
			}

			m_scene.HandleSDLEvents(e);
			m_imGuiLayer.HandleEvent(&e);
		}

		// don't draw if the window is minimized
		if (stopRendering) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		m_scene.Update(m_stats);
		m_renderer.Render(m_imGuiLayer);

		// get clock again and compare with start clock
		auto end = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		m_stats.frameTime = elapsed.count() / 1000.f;
		m_stats.fps = 1000.0f / m_stats.frameTime;
	}
}

void ThirdEngine::Cleanup()
{
	vkDeviceWaitIdle(m_vulkanContext.GetDevice());

	m_renderer.Cleanup();
	m_AssetManager.Cleanup();
	m_vulkanContext.Cleanup();
	m_renderWindow.Cleanup();
}