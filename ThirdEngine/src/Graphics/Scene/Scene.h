#pragma once

#include "../vk_Types.h"
#include "../../Camera/Camera.h"
#include "../../Window/Window.h"
#include "../Asset/AssetManager.h"

#include <unordered_map>

class Scene
{
public:
	void Init(Window* window, AssetManager* assetManager);
	void Update(EngineStats& stats);

	void HandleSDLEvents(const SDL_Event& e);

	GPUSceneData GetGPUSceneData() { return m_sceneData; }
	DrawContext mainDrawContext;

private:
	AssetManager* m_pAssetManager;

	// Scene Data
	GPUSceneData m_sceneData;

	// Camera
	Camera m_mainCamera;
	float zNear = 0.1f;
	float zFar = 1000.0f;
};

