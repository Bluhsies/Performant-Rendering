#include "scripts/include/camera.hpp"
#include <entt/entt.hpp>

void CameraScript::onUpdate(float timestep)
{
	bool recalc = false;

	auto& transformComp = m_registry.get<Transform>(m_entity);
	glm::vec3 forward = { -transformComp.transform[2][0], -transformComp.transform[2][1], -transformComp.transform[2][2] };
	glm::vec3 right = { transformComp.transform[0][0], transformComp.transform[0][1], transformComp.transform[0][2] };

	if (m_winRef.doIsKeyPressed(GLFW_KEY_W)) { transformComp.translation += forward * m_movementSpeed.z * timestep; recalc = true; }
	if (m_winRef.doIsKeyPressed(GLFW_KEY_A)) { transformComp.translation -= right * m_movementSpeed.x * timestep; recalc = true; }
	if (m_winRef.doIsKeyPressed(GLFW_KEY_S)) { transformComp.translation -= forward * m_movementSpeed.z * timestep; recalc = true; }
	if (m_winRef.doIsKeyPressed(GLFW_KEY_D)) { transformComp.translation += right * m_movementSpeed.x * timestep; recalc = true; }
	if (m_winRef.doIsKeyPressed(GLFW_KEY_UP)) { transformComp.translation.y += m_movementSpeed.y * timestep; recalc = true; }
	if (m_winRef.doIsKeyPressed(GLFW_KEY_DOWN)) { transformComp.translation.y -= m_movementSpeed.y * timestep; recalc = true; }


	if (m_winRef.doIsKeyPressed(GLFW_KEY_LEFT)) {
		glm::vec3 EulerAngles = glm::eulerAngles(transformComp.rotation);
		EulerAngles.y += m_turnSpeed * timestep;
		transformComp.rotation = glm::quat(EulerAngles);
		recalc = true;
	}

	if (m_winRef.doIsKeyPressed(GLFW_KEY_RIGHT)) {
		glm::vec3 EulerAngles = glm::eulerAngles(transformComp.rotation);
		EulerAngles.y -= m_turnSpeed * timestep;
		transformComp.rotation = glm::quat(EulerAngles);
		recalc = true;
	}

	if (recalc) transformComp.recalc();
}