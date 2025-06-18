/** \file physics */
#pragma once

#include "core/planeSweep.hpp"
#include "rendering/scene.hpp"
//#include "gameObjects/collidable.hpp"
#include "components/colliders.hpp"
#include "components/transform.hpp"


namespace Physics
{
	float DistanceOBBToPoint(Scene* scene, const OBBCollider& obb, const glm::vec3& point); //!< Distance between an OBB and a point
	float DistanceOBBToSphere(Scene* scene, const OBBCollider& obb, const SphereCollider& sphere); //!< Distance between an OBB and a sphere
}

using AABB = std::pair<glm::vec3, glm::vec3>;

/** \class BroadPhase
*	\brief BroadPhase physics. Maintains a subset of entities which need checking for collisions
*/

class BroadPhase
{
public:
	BroadPhase() = default; //!< Default constructor
	void init(std::shared_ptr<Scene> scene); //!< Initialise all internal structures
	void onUpdate(float timestep); //!< Runs once per frame
public:
	std::vector<entt::entity> OOBcandidates; //!< Candidate entities for collision checks
	std::vector<entt::entity> sphereCandidates; //!< Candidate entities for collision checks
	void erase(entt::entity entity); //!< Erase entity from the broadphase model

	const std::unordered_map<entt::entity, AABB>& getBoxColliderAABBs() const { return m_BoxColliderAABBs; } //!< A getter for the box collider AABBS
	const std::unordered_map<entt::entity, AABB>& getSphereColliderAABBs() const { return m_SphereColliderAABBs; } //!< A getter for the sphere collider AABBS

private:
	std::unordered_map<entt::entity, AABB> m_BoxColliderAABBs; //!< AABBs for entities with OBB colliders
	std::unordered_map<entt::entity, AABB> m_SphereColliderAABBs; //!< AABBs for entities with sphere colliders
	std::shared_ptr<Scene> m_scene; //!< Create a shared pointer for the main scene

	PlaneSweep m_OBBPlaneSweep; //!< Create a planesweep for the OBBs
	PlaneSweep m_spherePlaneSweep; //!< Create a planesweep for the sphere colliders

};