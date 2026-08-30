#pragma once

#include "../Graphics/Types.h"
#include <../../vendor/include/SDL/SDL_events.h>

class Camera {

public:

	void Init(glm::vec3 position, glm::vec3 velocity);

	void SetPosition(glm::vec3 position) { m_position = position; }
	void SetVelocity(glm::vec3 velocity) { m_velocity = velocity; }

	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetRotationMatrix() const;
	glm::vec3 GetPosition() { return m_position; }

	void ProcessSDLEvent(const SDL_Event& e);

	void Update();

private:
	glm::vec3 m_velocity;
	glm::vec3 m_position;

	// vertical rotation
	float pitch{ 0.f };
	// horizontal rotation
	float yaw{ 0.f };
};


