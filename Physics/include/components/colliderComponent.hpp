#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Rock
{
	struct OBBComponent
	{
		OBBComponent(const glm::vec3 halfExtents)
			: m_halfExtents(halfExtents) {}

		glm::vec3 m_halfExtents;
	};

	struct SphereComponent
	{
		SphereComponent(const float radius)
			: m_radius(radius) {}

		float m_radius;
	};
}
