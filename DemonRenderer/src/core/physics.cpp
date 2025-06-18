/** \file physics.cpp */

#include "core/physics.hpp"
#include <iostream>


namespace Physics
{
	/* Implementation is based Real - Time Collision Detection by Christer Ericson pp 133
	*  Calculate the closest point on or inside the OBB.
	*  Distance is calculate from that.
	*/
	float DistanceOBBToPoint(Scene* scene, const OBBCollider& obb, const glm::vec3& point)
	{
		//auto& obbActor = scene->m_actors.at(obb.actorIdx);

		auto& obbTransform = scene->m_entities.get<Transform>(obb.entity);

		// ClosestPoint is the closest point inside or on the interior of the OBB
		glm::vec3 closestPoint = obbTransform.translation; // Set closest point to actor position
		glm::vec3 centre = point - obbTransform.translation; // Move the problem so actor is at world centre

		// Get axis of OBB
		glm::vec3 OBBRight = { obbTransform.transform[0][0], obbTransform.transform[0][1], obbTransform.transform[0][2] };
		glm::vec3 OBBUp = { obbTransform.transform[1][0], obbTransform.transform[1][1], obbTransform.transform[1][2] };
		glm::vec3 OBBForward = { -obbTransform.transform[2][0], -obbTransform.transform[2][1], -obbTransform.transform[2][2] };

		// Project along each axis, moving closest point as we go

		// Project along X
		float distX = glm::dot(centre, OBBRight);
		// Clamp to OBB half extents
		distX = std::clamp(distX, -obb.halfExtents.x, obb.halfExtents.x);
		// Move closest point
		closestPoint += OBBRight * distX;

		// Project along Y
		float distY = glm::dot(centre, OBBUp);
		// Clamp to OBB half extents
		distY = std::clamp(distY, -obb.halfExtents.y, obb.halfExtents.y);
		// Move closest point
		closestPoint += OBBUp * distY;

		// Project along Z
		float distZ = glm::dot(centre, OBBForward);
		// Clamp to OBB half extents
		distZ = std::clamp(distZ, -obb.halfExtents.z, obb.halfExtents.z);
		// Move closest point
		closestPoint += OBBUp * distZ;

		// Find the distance squared
		float distanceSquared = glm::dot(closestPoint - point, closestPoint - point);

		return sqrtf(distanceSquared);

	}
	float DistanceOBBToSphere(Scene* scene, const OBBCollider& obb, const SphereCollider& sphere)
	{
		auto& sphereCentre = scene->m_entities.get<Transform>(sphere.entity).translation;
		return DistanceOBBToPoint(scene, obb, sphereCentre) - sphere.radius;

	}
}

void BroadPhase::init(std::shared_ptr<Scene> scene)
{
	/*
	Init function is used to generate the AABBs for each entity which has a collider within the game. This is called in the main
	game after the generateLevel function.
	*/

	m_scene = scene;
	m_OBBPlaneSweep = PlaneSweep(glm::vec2(0.5f, 0.2f));
	m_spherePlaneSweep = PlaneSweep(glm::vec2(0.5f, 0.2f));

	m_BoxColliderAABBs.clear();
	m_SphereColliderAABBs.clear();

	// Setup AABBs for OBB colliders
	auto viewOBB = m_scene->m_entities.view<OBBCollider, Transform>();
	for (auto entity : viewOBB)
	{
		auto& obb = viewOBB.get<OBBCollider>(entity);
		auto& transform = viewOBB.get<Transform>(entity);

		glm::vec3 min = transform.translation - obb.halfExtents;
		glm::vec3 max = transform.translation + obb.halfExtents;

		m_BoxColliderAABBs[entity] = { min, max };

		m_OBBPlaneSweep.addEntity(entity, { min, max }); // Add entity to the plane sweep for OBB

		m_scene->m_entities.emplace<AABB>(entity, min, max);
	}

	// Setup AABBs for Sphere colliders
	auto viewSphere = m_scene->m_entities.view<SphereCollider, Transform>();
	for (auto entity : viewSphere)
	{
		auto& sphere = viewSphere.get<SphereCollider>(entity);
		auto& transform = viewSphere.get<Transform>(entity);

		glm::vec3 min = transform.translation - glm::vec3(sphere.radius);
		glm::vec3 max = transform.translation + glm::vec3(sphere.radius);

		m_SphereColliderAABBs[entity] = { min, max };

		m_spherePlaneSweep.addEntity(entity, { min, max }); // Add entity to the plane sweep for spheres

		m_scene->m_entities.emplace<AABB>(entity, min, max);
	}

}

void BroadPhase::onUpdate(float timestep)
{
	
	
	if (!m_scene) return;

	// Clear the candidate lists before updating
	OOBcandidates.clear();
	sphereCandidates.clear();

	// Iterate through entities in the scene with AABB component
	auto view = m_scene->m_entities.view<AABB>();
	for (auto entity : view)
	{
		// Get the AABB of the current entity
		auto& aabb = view.get<AABB>(entity);

		// Handle OBB collisions with m_OBBPlaneSweep
		auto closeEntitiesOBB = m_OBBPlaneSweep.getCloseEntities(aabb);
		for (auto closeEntity : closeEntitiesOBB)
		{
			if (m_BoxColliderAABBs.count(closeEntity))
				if (std::find(OOBcandidates.begin(), OOBcandidates.end(), closeEntity) == OOBcandidates.end())
				{
					OOBcandidates.push_back(closeEntity);
				}
		}

		// Handle Sphere collisions with m_spherePlaneSweep
		auto closeEntitiesSphere = m_spherePlaneSweep.getCloseEntities(aabb);
		for (auto closeEntity : closeEntitiesSphere)
		{
			if (m_SphereColliderAABBs.count(closeEntity))
			{
				if (std::find(sphereCandidates.begin(), sphereCandidates.end(), closeEntity) == sphereCandidates.end())
				{
					sphereCandidates.push_back(closeEntity);
				}
			}
		}
	}
	

}

void BroadPhase::erase(entt::entity entity)
{

	
	m_OBBPlaneSweep.eraseEntity(entity); // Remove the entity from the OBB planesweep
	m_spherePlaneSweep.eraseEntity(entity); // Remove the entity from the sphere planesweep
	
	OOBcandidates.erase(std::remove(OOBcandidates.begin(), OOBcandidates.end(), entity), OOBcandidates.end()); // Remove the OBB candidates
	sphereCandidates.erase(std::remove(sphereCandidates.begin(), sphereCandidates.end(), entity), sphereCandidates.end()); // Remove the sphere candidates
		
	m_BoxColliderAABBs.erase(entity); // Remove the entity from the box collider AABBs
	m_SphereColliderAABBs.erase(entity); // Remove the entity from the sphere collider AABBs

}