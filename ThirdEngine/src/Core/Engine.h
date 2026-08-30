#pragma once

#include "Graphics/Backend/VkContext.h"
#include "Graphics/Renderer/Renderer.h"
#include "Graphics/Asset/AssetManager.h"
#include "Scene/Scene.h"
#include "UI/ImGuiLayer.h"
#include "Window/Window.h"
#include "Types.h"

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