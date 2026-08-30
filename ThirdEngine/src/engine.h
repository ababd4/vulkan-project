#pragma once

#include "Graphics/vk_Context.h"
#include "Graphics/vk_Renderer.h"
#include "Graphics/Asset/AssetManager.h"
#include "Graphics/Scene/Scene.h"
#include "Graphics/UI/ImGuiLayer.h"
#include "Window/Window.h"
#include "Graphics/vk_types.h"

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