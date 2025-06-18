#pragma once

#include "DemonRenderer.hpp"

class CameraScript : public Script
{
public:
	CameraScript(entt::entity& entity, std::shared_ptr<Scene> scene, GLFWWindowImpl& win, const glm::vec3& movementSpeed, float turnSpeed) :
		Script(entity, scene),
		m_winRef(win),
		m_movementSpeed(movementSpeed),
		m_turnSpeed(turnSpeed)
	{}
	virtual void onUpdate(float timestep) override;
	virtual void onKeyPress(KeyPressedEvent& e) override {};
	virtual void onKeyRelease(KeyReleasedEvent& e) override {};
private:
	GLFWWindowImpl& m_winRef;
	glm::vec3 m_movementSpeed{ 1.f, 1.f, 1.f };
	float m_turnSpeed{ 0.5f };
};