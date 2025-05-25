#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Rock
{
	struct RigidbodyComponent
	{
		RigidbodyComponent(const float mass, const glm::vec3& v = glm::vec3(0.f),
			const glm::vec3& a = glm::vec3(0.f), const bool gravity = true, const bool grounded = true)
			: m_velocity(v), m_acceleration(a), m_mass(mass), m_gravity(gravity), m_grounded(grounded) {}

		void update(float deltaTime)
		{
			// update acceleration
			if (m_grounded)
			{
				m_acceleration.y = 0.f;
				m_velocity.y = 0.f;
			}
			else addForce(gravity * deltaTime);
			// update velocity
			m_velocity += m_acceleration * deltaTime;
		}

		void addForce(const glm::vec3& force)
		{
			this->m_acceleration += force / this->m_mass;
		}

		const glm::vec3 gravity = glm::vec3(0.f, -9.8f, 0.f);
		glm::vec3 m_velocity;
		glm::vec3 m_acceleration;
		float m_mass;
		bool m_gravity;
		bool m_grounded;
	};
}
