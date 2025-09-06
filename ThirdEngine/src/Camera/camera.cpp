#include "camera.h"

#include <../../vendor/include/glm/glm.hpp>
#include <../../vendor/include/glm/gtx/transform.hpp>
#include <../../vendor/include/glm/gtx/quaternion.hpp>

void Camera::Init(glm::vec3 position, glm::vec3 velocity) 
{
	SetPosition(position);
	SetVelocity(velocity);
}

void Camera::Update()
{
	glm::mat4 cameraRotation = GetRotationMatrix();
	m_position += glm::vec3(cameraRotation * glm::vec4(m_velocity * 0.1f, 0.f));
}

void Camera::ProcessSDLEvent(const SDL_Event& e)
{
	if (e.type == SDL_KEYDOWN) {
		if (e.key.keysym.sym == SDLK_w) { m_velocity.z = -1; }
		if (e.key.keysym.sym == SDLK_a) { m_velocity.x = -1; }
		if (e.key.keysym.sym == SDLK_s) { m_velocity.z = 1; }
		if (e.key.keysym.sym == SDLK_d) { m_velocity.x = 1; }
	}

	if (e.type == SDL_KEYUP) {
		if (e.key.keysym.sym == SDLK_w) { m_velocity.z = 0; }
		if (e.key.keysym.sym == SDLK_a) { m_velocity.x = 0; }
		if (e.key.keysym.sym == SDLK_s) { m_velocity.z = 0; }
		if (e.key.keysym.sym == SDLK_d) { m_velocity.x = 0; }
	}

	if (e.type == SDL_MOUSEMOTION) {
		yaw += (float)e.motion.xrel / 200.f;
		pitch -= (float)e.motion.yrel / 200.f;

		// make sure that when pitch is out of bounds, screen doesn't get flipped
		if (pitch > 50.0f)
			pitch = 50.0f;
		if (pitch < -50.0f)
			pitch = -50.0f;
	}
}

glm::mat4 Camera::GetViewMatrix() const
{
	// to create a correct model view, we need to move the world in opposite
	// direction to the camera
	//  so we will create the camera model matrix and invert
	glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), m_position);
	glm::mat4 cameraRotation = GetRotationMatrix();
	return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::GetRotationMatrix() const
{
	// fairly typical FPS style camera. we join the pitch and yaw rotations into
	// the final rotation matrix

	glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{ 1.f, 0.f, 0.f });
	glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{ 0.f, -1.f, 0.f });

	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}