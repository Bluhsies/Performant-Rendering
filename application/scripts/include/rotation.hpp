#pragma once

#include <GLFW/glfw3.h>
#include "components/script.hpp"
#include "components/transform.hpp"

class RotationScript : public Script
{
public:
	//Added entity and scene to the constructor input parameters instead of actor.
	RotationScript(entt::entity entity, std::shared_ptr<Scene> scene, const glm::vec3& rotSpeed, uint32_t pauseKeyMapping) : 
		Script(entity, scene), // added entity and scene instead of actor
		m_rotSpeed(rotSpeed),
		m_pauseKey(pauseKeyMapping)
	{}
	virtual void onUpdate(float timestep) override;
	virtual void onKeyPress(KeyPressedEvent& e) override;
	virtual void onKeyRelease(KeyReleasedEvent& e) override {};
private:
	glm::vec3 m_rotSpeed{0.f, 0.f, 0.f };
	uint32_t m_pauseKey{ GLFW_KEY_P };
	bool m_paused{ false };
};


