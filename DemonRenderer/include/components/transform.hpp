/** \file transform.hpp */
#pragma once
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

/** \struct Transform
* \brief A transform component*/

struct Transform
{

	glm::mat4 transform{ glm::mat4{1.f} }; //Overall transform
	glm::quat rotation{ glm::quat(glm::vec3(0.f)) }; // Orientation as a quaternion
	glm::vec3 translation{ glm::vec3(0.f) }; //Translation such as position
	glm::vec3 scale{ glm::vec3(1.f) }; //Scale

	Transform() = default; //default constructor

	Transform
	(const glm::vec3& t, const glm::vec3& r, const glm::vec3 s) :
		translation(t),
		rotation(glm::quat(r)),
		scale(s)
	{
		recalc();
	}

	void recalc()
	{

		glm::mat4 t = glm::translate(glm::mat4(1.f), translation);
		glm::mat4 r = glm::toMat4(rotation);
		glm::mat4 s = glm::scale(glm::mat4(1.f), scale);

		transform = t * r * s;
	}

	void recalc(const glm::mat4& parent)
	{

		glm::mat4 t = glm::translate(glm::mat4(1.f), translation);
		glm::mat4 r = glm::toMat4(rotation);
		glm::mat4 s = glm::scale(glm::mat4(1.f), scale);

		transform = parent * t * r * s;

	}

};