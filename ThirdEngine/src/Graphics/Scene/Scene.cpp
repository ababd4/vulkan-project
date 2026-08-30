#include "Scene.h"

#include "../../Graphics/Loader/GltfLoader.h"

#include <../../vendor/include/glm/gtx/transform.hpp>

void Scene::Init(Window* window, AssetManager* assetManager)
{
	m_pAssetManager = assetManager;

	m_mainCamera.Init(glm::vec3(0.f, 0.f, 10.f), glm::vec3(0.f));

	m_sceneData.proj = glm::perspective(glm::radians(70.f), (float)window->GetWindowExtent().width / (float)window->GetWindowExtent().height, zNear, zFar);
	m_sceneData.proj[1][1] *= -1;
}

void Scene::Update(EngineStats& stats)
{
	// begin clock
	auto start = std::chrono::system_clock::now();

	// Update Camera
	m_mainCamera.Update();
	m_sceneData.cameraPos = glm::vec4(m_mainCamera.GetPosition(), 1.f);

	// Update camera matrix
	m_sceneData.view = m_mainCamera.GetViewMatrix();
	m_sceneData.viewproj = m_sceneData.proj * m_sceneData.view;

	// change direction
	float time = SDL_GetTicks() * 0.001f;
	float angle = time * 0.5f;

	m_sceneData.sunlightColor = { 1.f, 0.58f, 0.3f, 1.f };
	//m_sceneData.sunlightDirection = { cos(angle), -1.f, sin(angle), 1.f };
	m_sceneData.sunlightDirection = glm::vec4{glm::normalize(glm::vec3(-1.f, -0.22f, 0.35f)), 4.f};
	m_sceneData.ambientColor = { 0.025f, 0.04f, 0.07f, 1.f };

	// lightView for shadow mapping
	glm::vec3 sceneCenter = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 lightDirection = glm::normalize(glm::vec3(m_sceneData.sunlightDirection));
	float lightDistance = 70.f;
	glm::vec3 lightPosition = sceneCenter - lightDirection * lightDistance;
	glm::vec3 lightUp = glm::vec3(0.f, 0.f, 1.f);
	glm::mat4 lightView = glm::lookAt(lightPosition, sceneCenter, lightUp);

	float shadowHalfWidth = 30.f;
	float shadowHalfHeight = 30.f;

	float shadowNear = 50.f;
	float shadowFar = 150.F;

	glm::mat4 lightProjection = glm::ortho(
		-shadowHalfWidth,
		shadowHalfWidth,
		-shadowHalfHeight,
		shadowHalfHeight,
		shadowNear,
		shadowFar
	);
	lightProjection[1][1] *= -1.f;

	m_sceneData.lightViewProj = lightProjection * lightView;

	mainDrawContext.OpaqueSurfaces.clear();
	mainDrawContext.TransparentSurfaces.clear();
	

	/*float cubeDistance = 5.f;
	for (uint32_t i = 0; i < 3; i++) {
		for (uint32_t j = 0; j < 3; j++) {
			for (uint32_t k = 0; k < 3; k++) {
				model = glm::translate(glm::mat4{ 1.f }, glm::vec3(cubeDistance * i, cubeDistance * j + 5.0f, cubeDistance * k));
				m_pAssetManager->GetModelByName("cube")->Draw(model, mainDrawContext);
			}
		}
	}*/

	//glm::mat4 model = glm::translate(glm::mat4{ 1.f }, glm::vec3(0.f, -2.0f, 0.f));
	//glm::scale(model, glm::vec3(3.f));
	//m_pAssetManager->GetModelByName("case")->Draw(model, mainDrawContext);
	//model = glm::scale(glm::mat4{ 1.f }, glm::vec3(5.f));
	//m_pAssetManager->GetModelByName("teapot1")->Draw(model, mainDrawContext);
	//m_pAssetManager->GetModelByName("teapot2")->Draw(glm::translate(model, glm::vec3(2.5f, 0.0f, 0.f)), mainDrawContext);

	m_pAssetManager->GetModelByName("helmet")->Draw(glm::translate(glm::mat4{ 1.f }, glm::vec3(0.f, 2.f, 0.f)), mainDrawContext);
	m_pAssetManager->GetModelByName("sponza")->Draw(glm::mat4{ 1.f }, mainDrawContext);


	// get clock again and compare with start clock
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	stats.sceneUpdateTime = elapsed.count() / 1000.0f;
}

void Scene::HandleSDLEvents(const SDL_Event& e)
{
	m_mainCamera.ProcessSDLEvent(e);
}