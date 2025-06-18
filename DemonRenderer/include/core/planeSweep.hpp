#pragma once
#include <vector>
#include <algorithm>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

using AABB = std::pair<glm::vec3, glm::vec3>;

/** \class PlaneSweep
*	\brief A broad phase approach which looks at everything with a window, and intervals along the negative z-axis
*
*/

class PlaneSweep
{

public:

	PlaneSweep() = default;
	PlaneSweep(glm::vec2 windowSize) : m_window(windowSize) {} //!< Constructor which takes a window

	const std::vector<entt::entity>& getCloseEntities(const AABB& ship); //!< Gets entities in the interval
	void addEntity(entt::entity entity, const AABB& entityAABB); //!< Add entity to the data structures
	void eraseEntity(entt::entity entity); //!< Erase from the data structures

private:

	std::vector<std::pair<float, entt::entity>> m_startPoints; //!< Points when entities start in the z-axis
	std::vector<std::pair<float, entt::entity>> m_endPoints; //!< Points when entities end in the z-axis
	std::vector<entt::entity> m_closeEntities; //!< Vector of entities currently in the window

	std::vector<std::pair<float, entt::entity>>::iterator m_startPosition{ m_startPoints.begin() }; //!< Position in start points
	std::vector<std::pair<float, entt::entity>>::iterator m_endPosition{ m_endPoints.begin() }; //!< Position in end points

	glm::vec2 m_window{ 0.5f, 0.2f }; //!< Window to be used for the planesweep

};