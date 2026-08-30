#pragma once

#include "Graphics/VkContext.h"
#include "Graphics/Renderer.h"
#include "Graphics/Asset/AssetManager.h"
#include "Graphics/Scene/Scene.h"
#include "Graphics/UI/ImGuiLayer.h"
#include "Window/Window.h"
#include "Graphics/types.h"

class ThirdEngine
{
public:

	void Init();
	void Run();
	void Cleanup();

private:

	bool stopRendering = false;

	VulkanContext m_vulkanContext;
	AssetManager m_AssetManager;
	Renderer m_renderer;
	Scene m_scene;
	Window m_renderWindow;
	ImGuiLayer m_imGuiLayer;
	EngineStats m_stats = {};
};