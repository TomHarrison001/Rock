#pragma once

#include "transform.hpp"
#include <algorithm>

namespace Rock
{
	struct OBBCollider
	{
		Transform transform;
		glm::vec3 halfExtents;
		OBBCollider() = default;
		OBBCollider(Transform transform) : transform(transform)
		{
			halfExtents = transform.scale / 2.f;
		};
	};

	struct SphereCollider
	{
		Transform transform;
		float radius;
		SphereCollider() = default;
		SphereCollider(Transform transform, float radius) : transform(transform), radius(radius) {};
	};

	static float distanceOBBtoPoint(OBBCollider& obb, glm::vec3& point)
	{
		glm::vec3 closestPoint = obb.transform.translation;
		glm::vec3 centre = point - obb.transform.translation;

		glm::vec3 OBBRight = { obb.transform.transform[0][0], obb.transform.transform[0][1], obb.transform.transform[0][2] };
		glm::vec3 OBBUp = { obb.transform.transform[1][0], obb.transform.transform[1][1], obb.transform.transform[1][2] };
		glm::vec3 OBBForward = { obb.transform.transform[2][0], obb.transform.transform[2][1], obb.transform.transform[2][2] };

		// project along x
		float distX = glm::dot(centre, OBBRight);
		// clamp to OBB half extents
		distX = std::clamp(distX, -obb.halfExtents.x, obb.halfExtents.x);
		// move closest point
		closestPoint += OBBRight * distX;

		// project along y
		float distY = glm::dot(centre, OBBUp);
		// clamp to OBB half extents
		distY = std::clamp(distY, -obb.halfExtents.y, obb.halfExtents.y);
		// move closest point
		closestPoint += OBBUp * distY;

		// project along z
		float distZ = glm::dot(centre, OBBForward);
		// clamp to OBB half extents
		distZ = std::clamp(distZ, -obb.halfExtents.z, obb.halfExtents.z);
		// move closest point
		closestPoint += OBBForward * distZ;

		float distanceSquared = glm::dot(closestPoint - point, closestPoint - point);
		return sqrtf(distanceSquared);
	}

	static float distanceOBBtoSphere(OBBCollider& obb, SphereCollider& sphere)
	{
		glm::vec3 sphereCentre = sphere.transform.translation;
		return distanceOBBtoPoint(obb, sphereCentre) - sphere.radius;
	}

	static float distanceSphereToPoint(SphereCollider& sphere, glm::vec3& point)
	{
		float dist = sqrtf(pow(sphere.transform.translation.x - point.x, 2) + pow(sphere.transform.translation.y - point.y, 2) + pow(sphere.transform.translation.z - point.z, 2));
		return dist - sphere.radius;
	}

	static float distanceSphereToSphere(SphereCollider& s1, SphereCollider& s2)
	{
		float dist = distanceSphereToPoint(s1, s2.transform.translation);
		return dist - s2.radius;
	}

	static bool getSeparatingPlane(const glm::vec3& dist, const glm::vec3& plane, const OBBCollider& obb1, const OBBCollider& obb2)
	{
		glm::mat4 rotA = glm::toMat4(obb1.transform.rotation);
		glm::mat4 rotB = glm::toMat4(obb2.transform.rotation);

		float lhs = abs(glm::dot(dist, plane));
		float rhs =
			abs(glm::dot(glm::vec3(rotA[0][0] * obb1.halfExtents.x, rotA[0][1] * obb1.halfExtents.x, rotA[0][2] * obb1.halfExtents.x), plane)) +
			abs(glm::dot(glm::vec3(rotA[1][0] * obb1.halfExtents.y, rotA[1][1] * obb1.halfExtents.y, rotA[1][2] * obb1.halfExtents.y), plane)) +
			abs(glm::dot(glm::vec3(rotA[2][0] * obb1.halfExtents.z, rotA[2][1] * obb1.halfExtents.z, rotA[2][2] * obb1.halfExtents.z), plane)) +
			abs(glm::dot(glm::vec3(rotB[0][0] * obb2.halfExtents.x, rotB[0][1] * obb2.halfExtents.x, rotB[0][2] * obb2.halfExtents.x), plane)) +
			abs(glm::dot(glm::vec3(rotB[1][0] * obb2.halfExtents.y, rotB[1][1] * obb2.halfExtents.y, rotB[1][2] * obb2.halfExtents.y), plane)) +
			abs(glm::dot(glm::vec3(rotB[2][0] * obb2.halfExtents.z, rotB[2][1] * obb2.halfExtents.z, rotB[2][2] * obb2.halfExtents.z), plane));
		return (lhs > rhs);
	}

	static bool obbIntersectingOBB(OBBCollider obb1, OBBCollider obb2)
	{
		glm::vec3 dist = obb2.transform.translation - obb1.transform.translation;
		glm::mat4 rotA = glm::toMat4(obb1.transform.rotation);
		glm::mat4 rotB = glm::toMat4(obb2.transform.rotation);

		return !(getSeparatingPlane(dist, glm::vec3(rotA[0][0], rotA[0][1], rotA[0][2]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotA[1][0], rotA[1][1], rotA[1][2]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotA[2][0], rotA[2][1], rotA[2][2]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotB[0][0], rotB[0][1], rotB[0][2]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotB[1][0], rotB[1][1], rotB[1][2]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotB[2][0], rotB[2][1], rotB[2][2]), obb1, obb2) ||

			getSeparatingPlane(dist, glm::vec3(rotA[0][0] * rotB[0][0], rotA[0][1] * rotB[0][1], rotA[0][2] * rotB[0][1]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotA[0][0] * rotB[1][0], rotA[0][1] * rotB[1][1], rotA[0][2] * rotB[1][1]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotA[0][0] * rotB[2][0], rotA[0][1] * rotB[2][1], rotA[0][2] * rotB[2][1]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotA[1][0] * rotB[0][0], rotA[1][1] * rotB[0][1], rotA[1][2] * rotB[0][1]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotA[1][0] * rotB[1][0], rotA[1][1] * rotB[1][1], rotA[1][2] * rotB[1][1]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotA[1][0] * rotB[2][0], rotA[1][1] * rotB[2][1], rotA[1][2] * rotB[2][1]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotA[2][0] * rotB[0][0], rotA[2][1] * rotB[0][1], rotA[2][2] * rotB[0][1]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotA[2][0] * rotB[1][0], rotA[2][1] * rotB[1][1], rotA[2][2] * rotB[1][1]), obb1, obb2) ||
			getSeparatingPlane(dist, glm::vec3(rotA[2][0] * rotB[2][0], rotA[2][1] * rotB[2][1], rotA[2][2] * rotB[2][1]), obb1, obb2)
		);
	}

	static bool obbIntersectingSphere(OBBCollider& obb, SphereCollider& sphere)
	{
		return distanceOBBtoSphere(obb, sphere) <= 0.f;
	}

	static bool sphereIntersectingSphere(SphereCollider& s1, SphereCollider& s2)
	{
		return distanceSphereToSphere(s1, s2) <= 0.f;
	}
}
