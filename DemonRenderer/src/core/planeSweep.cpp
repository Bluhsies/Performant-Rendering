/** \file planeSweep.cpp */
#include "core/planeSweep.hpp"

void PlaneSweep::addEntity(entt::entity entity, const AABB& entityAABB)
{

	float startZ = entityAABB.first.z;
	float endZ = entityAABB.second.z;

	m_startPoints.emplace_back(startZ, entity); // Adds a start point as the first point in the entities AABB and the entity
	m_endPoints.emplace_back(endZ, entity); // Adds an end point as the first point in the entities AABB and the entity

	std::sort(m_startPoints.begin(), m_startPoints.end()); // Sort the start points
	std::sort(m_endPoints.begin(), m_endPoints.end()); // Sort the end points

	m_startPosition = m_startPoints.begin(); // Set the start position as the first point in start points
	m_endPosition = m_endPoints.begin(); // Set the end position as the first point in end points

}

void PlaneSweep::eraseEntity(entt::entity entity)
{

	m_startPoints.erase(
		std::remove_if(m_startPoints.begin(), m_startPoints.end(), 
			[entity](const auto& pair) { return pair.second == entity; }),
		m_startPoints.end()); // All start points with the entity are removed

	m_endPoints.erase(
		std::remove_if(m_endPoints.begin(), m_endPoints.end(),
			[entity](const auto& pair) {return pair.second == entity; }),
		m_endPoints.end()); // All end points with the entity are removed

	m_closeEntities.clear(); // Clears close entities

	m_startPosition = m_startPoints.begin(); // Start position is reset as the first point in start points
	m_endPosition = m_endPoints.begin(); // End position is reset as the first point in the end points

}

const std::vector<entt::entity>& PlaneSweep::getCloseEntities(const AABB& ship)
{

	m_closeEntities.clear();

	/*
		Create the window, taking into account the ship AABB and the window of the plane sweep (m_window)
	*/
	float windowStart = ship.first.z - m_window.x;
	float windowEnd = ship.second.z + m_window.y;

	m_startPosition = m_startPoints.begin();
	m_endPosition = m_endPoints.begin();

	/*
		Check to see if the next start point is greater than or equal to the window start point.
		It will also check to see if m_startPosition is not equal to m_startPoints.end().
		If it is it will add the entity to close entities and increment the start position iterator.
	*/

	while (m_startPosition != m_startPoints.end() && m_startPosition->first <= windowEnd)
	{

		if (m_startPosition->first >= windowStart)
		{

			if (std::find(m_closeEntities.begin(), m_closeEntities.end(), m_startPosition->second) == m_closeEntities.end())
			{
				m_closeEntities.push_back(m_startPosition->second);
			}
		}
		++m_startPosition;
	}
	
	/*
		Check to see if the next end point is less than the window start point or greater than the window end point.
		It will also check to see if m_endPosition is not equal to m_endPoints.end().
		If it is it will add the entity to close entities and increment the end position iterator.
	*/

	while (m_endPosition != m_endPoints.end() && (m_endPosition->first < windowStart || m_endPosition->first > windowEnd)) //remove it if it's outside of the window
	{

		m_closeEntities.erase(std::remove(m_closeEntities.begin(), m_closeEntities.end(), m_endPosition->second),
			m_closeEntities.end());
		++m_endPosition;

	}

	return m_closeEntities; // Return the value of m_close entities

}
