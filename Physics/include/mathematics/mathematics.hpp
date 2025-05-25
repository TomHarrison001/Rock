#pragma once

#include <stdexcept>
#include <algorithm>
#include <glm/glm.hpp>

#include "../components/transformComponent.hpp"
#include "../components/colliderComponent.hpp"
#include "../components/rigidbodyComponent.hpp"

namespace Rock
{
	static float clamp(float x, float min, float max)
	{
		if (min == max)
			return min;
		if (min > max)
			throw new std::runtime_error("Min is greater than max.");
		if (x < min)
			return min;
		if (x > max)
			return max;
		return x;
	}

	static int clamp(int x, int min, int max)
	{
		float result = clamp(static_cast<float>(x), static_cast<float>(min), static_cast<float>(max));
		return static_cast<int>(result);
	}

	// v**2 = u**2 + 2as
	static glm::vec3 calculateVelocity(glm::vec3 u, glm::vec3 a, glm::vec3 s)
	{
		return sqrt(glm::vec3(pow(u.x, 2.f), pow(u.y, 2.f), pow(u.z, 2.f)) + 2.f * a * s);
	}

	// v = u + at
	static glm::vec3 calculateVelocity(glm::vec3 u, glm::vec3 a, float t)
	{
		return u + a * t;
	}

	// s = ut + 1/2 at**2
	static glm::vec3 calculatePosition(float t, glm::vec3 u, glm::vec3 a)
	{
		return u * t + (a * pow(t, 2.f)) / 2.f;
	}

	// s = vt - 1/2 at**2
	static glm::vec3 calculatePosition(glm::vec3 v, float t, glm::vec3 a)
	{
		return v * t - (a * pow(t, 2.f)) / 2.f;
	}

	// s = (v + u) / 2 * t
	static glm::vec3 calculatePosition(glm::vec3 v, glm::vec3 u, float t)
	{
		return (v + u) / 2.f * t;
	}

	// 1/2 at**2 + ut - s = 0
	static std::tuple<float, float> calculateTime(float u, float a, float s)
	{
		return std::tuple<float, float>{
			(-u + sqrt(pow(u, 2) + 2 * a * s)) / a,
				(-u - sqrt(pow(u, 2) + 2 * a * s)) / a };
	}
	
	// f = ma
	static glm::vec3 calculateForce(float m, glm::vec3 a)
	{
		return m * a;
	}

	// a = f / m
	static glm::vec3 calculateAcceleration(glm::vec3 f, float m)
	{
		return f / m;
	}

	static float distanceOBBtoPoint(entt::registry& entities, entt::entity& obbEntity, glm::vec3& point)
	{
		TransformComponent& obbTransform = entities.get<TransformComponent>(obbEntity);
		OBBComponent& obbCollider = entities.get<OBBComponent>(obbEntity);

		glm::vec3 closestPoint = obbTransform.m_translation;
		glm::vec3 centre = point - obbTransform.m_translation;

		glm::vec3 OBBRight = { obbTransform.m_transform[0][0], obbTransform.m_transform[0][1], obbTransform.m_transform[0][2] };
		glm::vec3 OBBUp = { obbTransform.m_transform[1][0], obbTransform.m_transform[1][1], obbTransform.m_transform[1][2] };
		glm::vec3 OBBForward = { obbTransform.m_transform[2][0], obbTransform.m_transform[2][1], obbTransform.m_transform[2][2] };

		// project along x
		float distX = glm::dot(centre, OBBRight);
		// clamp to OBB half extents
		distX = std::clamp(distX, -obbCollider.m_halfExtents.x, obbCollider.m_halfExtents.x);
		// move closest point
		closestPoint += OBBRight * distX;

		// project along y
		float distY = glm::dot(centre, OBBUp);
		// clamp to OBB half extents
		distY = std::clamp(distY, -obbCollider.m_halfExtents.y, obbCollider.m_halfExtents.y);
		// move closest point
		closestPoint += OBBUp * distY;

		// project along z
		float distZ = glm::dot(centre, OBBForward);
		// clamp to OBB half extents
		distZ = std::clamp(distZ, -obbCollider.m_halfExtents.z, obbCollider.m_halfExtents.z);
		// move closest point
		closestPoint += OBBForward * distZ;

		float distanceSquared = glm::dot(closestPoint - point, closestPoint - point);
		return sqrtf(distanceSquared);
	}

	static float distanceOBBtoSphere(entt::registry& entities, entt::entity& obbEntity, entt::entity& sphereEntity)
	{
		TransformComponent& sphereTransform = entities.get<TransformComponent>(sphereEntity);
		SphereComponent& sphereCollider = entities.get<SphereComponent>(sphereEntity);

		glm::vec3 sphereCentre = sphereTransform.m_translation;
		return distanceOBBtoPoint(entities, obbEntity, sphereCentre) - sphereCollider.m_radius;
	}

	static float distanceSphereToPoint(entt::registry& entities, entt::entity& sphereEntity, glm::vec3& point)
	{
		TransformComponent& sphereTransform = entities.get<TransformComponent>(sphereEntity);
		SphereComponent& sphereCollider = entities.get<SphereComponent>(sphereEntity);

		float dist = sqrtf(pow(sphereTransform.m_translation.x - point.x, 2) + pow(sphereTransform.m_translation.y - point.y, 2) + pow(sphereTransform.m_translation.z - point.z, 2));
		return dist - sphereCollider.m_radius;
	}

	static float distanceSphereToSphere(entt::registry& entities, entt::entity& sphere1Entity, entt::entity& sphere2Entity)
	{
		TransformComponent& sphere2Transform = entities.get<TransformComponent>(sphere2Entity);
		SphereComponent& sphere2Collider = entities.get<SphereComponent>(sphere2Entity);

		float dist = distanceSphereToPoint(entities, sphere1Entity, sphere2Transform.m_translation);
		return dist - sphere2Collider.m_radius;
	}

	static bool getSeparatingPlane(entt::registry& entities, entt::entity obb1Entity, entt::entity obb2Entity, const glm::vec3& dist, const glm::vec3& plane)
	{
		TransformComponent& obb1Transform = entities.get<TransformComponent>(obb1Entity);
		OBBComponent& obb1Collider = entities.get<OBBComponent>(obb1Entity);
		TransformComponent& obb2Transform = entities.get<TransformComponent>(obb2Entity);
		OBBComponent& obb2Collider = entities.get<OBBComponent>(obb2Entity);

		glm::mat4 rotA = glm::toMat4(obb1Transform.m_rotation);
		glm::mat4 rotB = glm::toMat4(obb2Transform.m_rotation);

		float lhs = abs(glm::dot(dist, plane));
		float rhs =
			abs(glm::dot(glm::vec3(rotA[0][0] * obb1Collider.m_halfExtents.x, rotA[0][1] * obb1Collider.m_halfExtents.x, rotA[0][2] * obb1Collider.m_halfExtents.x), plane)) +
			abs(glm::dot(glm::vec3(rotA[1][0] * obb1Collider.m_halfExtents.y, rotA[1][1] * obb1Collider.m_halfExtents.y, rotA[1][2] * obb1Collider.m_halfExtents.y), plane)) +
			abs(glm::dot(glm::vec3(rotA[2][0] * obb1Collider.m_halfExtents.z, rotA[2][1] * obb1Collider.m_halfExtents.z, rotA[2][2] * obb1Collider.m_halfExtents.z), plane)) +
			abs(glm::dot(glm::vec3(rotB[0][0] * obb2Collider.m_halfExtents.x, rotB[0][1] * obb2Collider.m_halfExtents.x, rotB[0][2] * obb2Collider.m_halfExtents.x), plane)) +
			abs(glm::dot(glm::vec3(rotB[1][0] * obb2Collider.m_halfExtents.y, rotB[1][1] * obb2Collider.m_halfExtents.y, rotB[1][2] * obb2Collider.m_halfExtents.y), plane)) +
			abs(glm::dot(glm::vec3(rotB[2][0] * obb2Collider.m_halfExtents.z, rotB[2][1] * obb2Collider.m_halfExtents.z, rotB[2][2] * obb2Collider.m_halfExtents.z), plane));
		return (lhs > rhs);
	}

    static bool obbIntersectingOBB(entt::registry& entities, entt::entity obb1Entity, entt::entity obb2Entity)
    {
		TransformComponent& obb1Transform = entities.get<TransformComponent>(obb1Entity);
		OBBComponent& obb1Collider = entities.get<OBBComponent>(obb1Entity);
		TransformComponent& obb2Transform = entities.get<TransformComponent>(obb2Entity);
		OBBComponent& obb2Collider = entities.get<OBBComponent>(obb2Entity);

        glm::vec3 dist = obb2Transform.m_translation - obb1Transform.m_translation;
        glm::mat4 rotA = glm::toMat4(obb1Transform.m_rotation);
        glm::mat4 rotB = glm::toMat4(obb2Transform.m_rotation);
        
        return !(
		    getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[0][0], rotA[0][1], rotA[0][2])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[1][0], rotA[1][1], rotA[1][2])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[2][0], rotA[2][1], rotA[2][2])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotB[0][0], rotB[0][1], rotB[0][2])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotB[1][0], rotB[1][1], rotB[1][2])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotB[2][0], rotB[2][1], rotB[2][2])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[0][0] * rotB[0][0], rotA[0][1] * rotB[0][1], rotA[0][2] * rotB[0][1])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[0][0] * rotB[1][0], rotA[0][1] * rotB[1][1], rotA[0][2] * rotB[1][1])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[0][0] * rotB[2][0], rotA[0][1] * rotB[2][1], rotA[0][2] * rotB[2][1])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[1][0] * rotB[0][0], rotA[1][1] * rotB[0][1], rotA[1][2] * rotB[0][1])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[1][0] * rotB[1][0], rotA[1][1] * rotB[1][1], rotA[1][2] * rotB[1][1])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[1][0] * rotB[2][0], rotA[1][1] * rotB[2][1], rotA[1][2] * rotB[2][1])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[2][0] * rotB[0][0], rotA[2][1] * rotB[0][1], rotA[2][2] * rotB[0][1])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[2][0] * rotB[1][0], rotA[2][1] * rotB[1][1], rotA[2][2] * rotB[1][1])) ||
            getSeparatingPlane(entities, obb1Entity, obb2Entity, dist, glm::vec3(rotA[2][0] * rotB[2][0], rotA[2][1] * rotB[2][1], rotA[2][2] * rotB[2][1]))
        );
    }

    static bool obbIntersectingSphere(entt::registry& entities, entt::entity& obbEntity, entt::entity& sphereEntity)
    {
        return distanceOBBtoSphere(entities, obbEntity, sphereEntity) <= 0.f;
    }

    static bool sphereIntersectingSphere(entt::registry& entities, entt::entity& sphere1Entity, entt::entity& sphere2Entity)
    {
        return distanceSphereToSphere(entities, sphere1Entity, sphere2Entity) <= 0.f;
    }
}
