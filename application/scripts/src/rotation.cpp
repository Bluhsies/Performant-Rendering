#include "scripts/include/rotation.hpp"

void RotationScript::onUpdate(float timestep)
{
	if (!m_paused) {
		auto& transformComp = m_registry.get<Transform>(m_entity);
		transformComp.rotation *= glm::quat(m_rotSpeed * timestep);
		transformComp.recalc();
	}
}

void RotationScript::onKeyPress(KeyPressedEvent& e)
{
	if (e.getKeyCode() == m_pauseKey) m_paused = !m_paused;
}