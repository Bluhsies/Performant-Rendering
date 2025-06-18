#pragma once
#include "windows/GLFWWindowImpl.hpp"
#include <glm/glm.hpp>
#include "core/log.hpp"
#include "components/script.hpp"
#include "components/transform.hpp"

class ControllerScript : public Script
{
public:
	ControllerScript(entt::entity entity, std::shared_ptr<Scene> scene, GLFWWindowImpl& win, entt::entity& followCamera, const glm::vec3& movementSpeed, const glm::vec3& offset, float* speed) :
		Script(entity, scene),
		m_winRef(win),
		m_followCamera(followCamera),
		m_movementSpeed(movementSpeed),
		m_offset(offset),
		m_speedOut(speed)
	{
		*m_speedOut = m_movementSpeed.z;
		onUpdate(0.f); // Initialise camera
	}
	void onUpdate(float timestep) override;
	void onKeyPress(KeyPressedEvent& e) override {};
	void onKeyRelease(KeyReleasedEvent& e) override {};
	void onImGuiRender() override;
private:
	glm::vec3 m_movementSpeed{ 0.f, 0.f, 0.f };
	glm::vec3 m_offset{ 0.f, 0.f, 0.f };
	float* m_speedOut{ nullptr };
	GLFWWindowImpl& m_winRef;
	entt::entity m_followCamera;	
	const float m_minSpeed{ -0.5f };
	const float m_maxSpeed{ -5.5f };
	
};
