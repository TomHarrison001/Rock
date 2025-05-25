#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Rock
{
	struct TransformComponent
	{
		TransformComponent(const glm::vec3& t, const glm::vec3& r, const glm::vec3 s)
			: m_translation(t), m_rotation(glm::quat(r)), m_scale(s)
		{ recalculate(); }

		void recalculate()
		{
			glm::mat4 t = glm::translate(glm::mat4(1.f), m_translation);
			glm::mat4 r = glm::toMat4(m_rotation);
			glm::mat4 s = glm::scale(glm::mat4(1.f), m_scale);

			m_transform = t * r * s;
		}

		glm::mat4 m_transform;
		glm::vec3 m_translation;
		glm::quat m_rotation;
		glm::vec3 m_scale;
	};
}
