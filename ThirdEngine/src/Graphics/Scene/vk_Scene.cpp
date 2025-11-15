#include "vk_Scene.h"

#include "../../Graphics/Loader/vk_GltfLoader.h"

#include <../../vendor/include/glm/gtx/transform.hpp>

void Scene::Init(Window* window)
{
	m_mainCamera.Init(glm::vec3(0.f, 0.f, 10.f), glm::vec3(0.f));

	m_sceneData.proj = glm::perspective(glm::radians(70.f), (float)window->GetWindowExtent().width / (float)window->GetWindowExtent().height, zNear, zFar);
	m_sceneData.proj[1][1] *= -1;
}

void Scene::Update()
{
	// Update Camera
	m_mainCamera.Update();

	// Update camera matrix
	m_sceneData.view = m_mainCamera.GetViewMatrix();
	m_sceneData.viewproj = m_sceneData.proj * m_sceneData.view;
}

void Scene::HandleSDLEvents(const SDL_Event& e)
{
	m_mainCamera.ProcessSDLEvent(e);
}