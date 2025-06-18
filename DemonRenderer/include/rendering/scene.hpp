/** \file scene.hpp */
#pragma once

#include "rendering/lights.hpp"
//#include "gameObjects/actor.hpp"
#include <entt/entt.hpp> //commented out actor.hpp and added entt.hpp as we are working with registries instead.

/** \struct Scene
*	\brief Holds everything which makes up a scene
*/
struct Scene
{
	entt::registry m_entities; //!< Entities
	std::vector<DirectionalLight> m_directionalLights; //!< Directional lights
	std::vector<PointLight> m_pointLights; //!< Point lights
	std::vector<SpotLight> m_spotLights; //!< Spot lights

	

};