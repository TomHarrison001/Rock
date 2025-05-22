#pragma once

#include "transform.hpp"

namespace Rock
{
	struct OBBCollider
	{
		glm::vec3 halfExtents;
		Transform transform;

		OBBCollider(Transform transform, glm::vec3 halfExtents) : transform(transform), halfExtents(halfExtents) {}

		bool getSeparatingPlane(const glm::vec3& dist, const glm::vec3& plane, const OBBCollider& other)
		{
			glm::mat4 rotA = glm::toMat4(this->transform.rotation);
			glm::mat4 rotB = glm::toMat4(other.transform.rotation);

			float lhs = abs(glm::dot(dist, plane));
			float rhs = 
				abs(glm::dot(glm::vec3(rotA[0][0] * this->halfExtents.x, rotA[0][1] * this->halfExtents.x, rotA[0][2] * this->halfExtents.x), plane)) +
				abs(glm::dot(glm::vec3(rotA[1][0] * this->halfExtents.y, rotA[1][1] * this->halfExtents.y, rotA[1][2] * this->halfExtents.y), plane)) +
				abs(glm::dot(glm::vec3(rotA[2][0] * this->halfExtents.z, rotA[2][1] * this->halfExtents.z, rotA[2][2] * this->halfExtents.z), plane)) +
				abs(glm::dot(glm::vec3(rotB[0][0] * other.halfExtents.x, rotB[0][1] * other.halfExtents.x, rotB[0][2] * other.halfExtents.x), plane)) +
				abs(glm::dot(glm::vec3(rotB[1][0] * other.halfExtents.y, rotB[1][1] * other.halfExtents.y, rotB[1][2] * other.halfExtents.y), plane)) +
				abs(glm::dot(glm::vec3(rotB[2][0] * other.halfExtents.z, rotB[2][1] * other.halfExtents.z, rotB[2][2] * other.halfExtents.z), plane));
			return (lhs > rhs);
		}

		bool intersecting(OBBCollider& other)
		{
			glm::vec3 dist = other.transform.translation - this->transform.translation;
			glm::mat4 rotA = glm::toMat4(this->transform.rotation);
			glm::mat4 rotB = glm::toMat4(other.transform.rotation);

			return !(getSeparatingPlane(dist, glm::vec3(rotA[0][0], rotA[0][1], rotA[0][2]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotA[1][0], rotA[1][1], rotA[1][2]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotA[2][0], rotA[2][1], rotA[2][2]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotB[0][0], rotB[0][1], rotB[0][2]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotB[1][0], rotB[1][1], rotB[1][2]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotB[2][0], rotB[2][1], rotB[2][2]), other) ||

				getSeparatingPlane(dist, glm::vec3(rotA[0][0] * rotB[0][0], rotA[0][1] * rotB[0][1], rotA[0][2] * rotB[0][1]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotA[0][0] * rotB[1][0], rotA[0][1] * rotB[1][1], rotA[0][2] * rotB[1][1]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotA[0][0] * rotB[2][0], rotA[0][1] * rotB[2][1], rotA[0][2] * rotB[2][1]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotA[1][0] * rotB[0][0], rotA[1][1] * rotB[0][1], rotA[1][2] * rotB[0][1]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotA[1][0] * rotB[1][0], rotA[1][1] * rotB[1][1], rotA[1][2] * rotB[1][1]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotA[1][0] * rotB[2][0], rotA[1][1] * rotB[2][1], rotA[1][2] * rotB[2][1]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotA[2][0] * rotB[0][0], rotA[2][1] * rotB[0][1], rotA[2][2] * rotB[0][1]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotA[2][0] * rotB[1][0], rotA[2][1] * rotB[1][1], rotA[2][2] * rotB[1][1]), other) ||
				getSeparatingPlane(dist, glm::vec3(rotA[2][0] * rotB[2][0], rotA[2][1] * rotB[2][1], rotA[2][2] * rotB[2][1]), other)
			);
		}
	};
}
