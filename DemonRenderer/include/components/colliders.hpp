/** \file colliders.hpp*/
#pragma once
#include <glm/glm.hpp>
#include <entt/entt.hpp>

/** \struct OBBCollider
*   \brief Properties of an oriented bounding box collider
*/

struct OBBCollider
{

	/** Constructor which takes half extents and entity*/
	OBBCollider(const glm::vec3& he, entt::entity entt) : halfExtents(he), entity(entt) {}
	glm::vec3 halfExtents{ glm::vec3(0.f) };
	entt::entity entity;

};

/** \struct SphereCollider
*   \brief Properties of a sphere collider
*/

struct SphereCollider
{

	/** Constructor which takes radii and entity */
	SphereCollider(float rad, entt::entity entt) : radius(rad), entity(entt) {}
	float radius{ 0.f };
	entt::entity entity;

};
