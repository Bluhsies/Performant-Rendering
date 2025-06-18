/** \file CameraFrustrum.cpp */
#include <rendering/cameraFrustum.hpp>

CameraFrustrum::CameraFrustrum(const Camera& cam)
{

	/*
	Overloaded constructor which takes in camera as a parameter. It calculates the frustum based on the
	camera's view and projection matrix.
	*/

	// Calculate the planes

	glm::mat4 viewProj = cam.projection * cam.view;

	m_planes[0] = glm::vec4(viewProj[3] + viewProj[0]);
	m_planes[1] = glm::vec4(viewProj[3] - viewProj[0]);
	m_planes[2] = glm::vec4(viewProj[3] + viewProj[1]);
	m_planes[3] = glm::vec4(viewProj[3] - viewProj[1]);
	m_planes[4] = glm::vec4(viewProj[3] + viewProj[2]);
	m_planes[5] = glm::vec4(viewProj[3] - viewProj[2]);

	// Calulcate the corners

	const glm::vec4 corners[] = {
		glm::vec4(-1, -1, -1, 1), glm::vec4(1, -1, -1, 1),
		glm::vec4(1,  1, -1, 1),  glm::vec4(-1,  1, -1, 1),
		glm::vec4(-1, -1,  1, 1), glm::vec4(1, -1,  1, 1),
		glm::vec4(1,  1,  1, 1),  glm::vec4(-1,  1,  1, 1)
	};

	const glm::mat4 invMVP = glm::inverse(cam.projection * cam.view);

	for (int i = 0; i != 8; i++) {
		const glm::vec4 q = invMVP * corners[i];
		m_corners[i] = q / q.w;
	}

}

bool CameraFrustrum::intersects(const AABB& aabb)
{

	/*
	Checks whether the AABB intersects with the camera frustum.
	If it does, it increases the value of r for either the planes or corners.
	In the event that the r value reaches 8, it returns a false value and the entity is culled.
	Otherwise, it returns true and draws the entity to the scene.
	*/

	for (int i = 0; i < 6; i++)
	{

		int r = 0;
		r += (dot(m_planes[i], glm::vec4(aabb.first.x, aabb.first.y, aabb.first.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(m_planes[i], glm::vec4(aabb.second.x, aabb.first.y, aabb.first.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(m_planes[i], glm::vec4(aabb.first.x, aabb.second.y, aabb.first.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(m_planes[i], glm::vec4(aabb.second.x, aabb.second.y, aabb.first.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(m_planes[i], glm::vec4(aabb.first.x, aabb.first.y, aabb.second.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(m_planes[i], glm::vec4(aabb.second.x, aabb.first.y, aabb.second.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(m_planes[i], glm::vec4(aabb.first.x, aabb.second.y, aabb.second.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(m_planes[i], glm::vec4(aabb.second.x, aabb.second.y, aabb.second.z, 1.0f)) < 0.0) ? 1 : 0;
		if (r == 8)
			return false;

	}

	int r = 0;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((m_corners[i].x > aabb.second.x) ? 1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((m_corners[i].x < aabb.first.x) ? 1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((m_corners[i].y > aabb.second.y) ? 1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((m_corners[i].y < aabb.first.y) ? 1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((m_corners[i].z > aabb.second.z) ? 1 : 0);
	if (r == 8)
		return false;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((m_corners[i].z < aabb.first.z) ? 1 : 0);
	if (r == 8)
		return false;

	return true;

}