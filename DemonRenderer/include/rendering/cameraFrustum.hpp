#pragma once

#include <array>
#include "camera.hpp"
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <entt/entt.hpp>
#include "core/physics.hpp"

/** \class CameraFrustrum
*	\brief CameraFrustrum class which generates a camera frustum to use for CPU culling.
*/

class CameraFrustrum
{

public:
	CameraFrustrum() = default; //!< Default constructor
	CameraFrustrum(const Camera& cam); //!< Overloaded constructor which takes in Camera as a parameter
	bool intersects(const AABB& aabb); //!< Boolean function which checks to see if an AABB intersects with the camera frustum

private:
	std::array<glm::vec4, 6> m_planes; //!< Array which holds the planes of the frustum
	std::array<glm::vec4, 8> m_corners; //!< Array which holds the coners of the frustum
};