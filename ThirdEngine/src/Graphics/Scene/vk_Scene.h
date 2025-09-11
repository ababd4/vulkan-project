#pragma once

#include "../vk_Types.h"
#include "../../Camera/Camera.h"
#include "../../Window/Window.h"

#include <unordered_map>

class Scene
{
public:
	void Init(Window* window);
	void LoadEntities();
	void Update();

	void HandleSDLEvents(const SDL_Event& e);

	GPUSceneData GetGPUSceneData() { return m_sceneData; }

private:
	// Entity
	std::unordered_map<std::string, Entity> m_entities;

	// Scene Data
	GPUSceneData m_sceneData;

	// Camera
	Camera m_mainCamera;
	float zNear = 0.1f;
	float zFar = 1000.0f;
};

