/** \file script.hpp */
#pragma once

#include "events/keyEvents.hpp"
#include <entt/entt.hpp>
#include "rendering/scene.hpp"
#include <memory>

class Script
{

public:
	Script(entt::entity entity, std::shared_ptr<Scene> scene) : m_entity(entity), m_registry(scene->m_entities) {}
	virtual void onUpdate(float timestep) = 0;
	virtual void onImGuiRender() {};
	virtual void onKeyPress(KeyPressedEvent& e) = 0;
	virtual void onKeyRelease(KeyReleasedEvent& e) = 0;

protected:
	//Create a reference to the registry set in the scene.
	entt::registry& m_registry;
	entt::entity m_entity;
};

/** \class ScriptComp
*    \brief A script component to be added to an entity and can have a script attached
*/

class ScriptComp
{

public:

	ScriptComp() = default;

	void onUpdate(float timestep) { if (m_script) m_script->onUpdate(timestep); }
	void onImGuiRender() { if (m_script) m_script->onImGuiRender(); }
	void onKeyPress(KeyPressedEvent& e) { if (m_script) m_script->onKeyPress(e); }
	void onKeyRelease(KeyReleasedEvent& e) { if (m_script) m_script->onKeyRelease(e); }

	template <typename T, typename ...Args> void attachScript(Args&& ...args)
	{
		T* ptr = new T(std::forward<Args>(args)...);
		m_script.reset(static_cast<Script*>(ptr));
	}

private:

	std::shared_ptr<Script> m_script{ nullptr };

};
