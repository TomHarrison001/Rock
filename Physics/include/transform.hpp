#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Rock
{
	struct Transform
	{
		glm::mat4 transform;
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;

		Transform() = default;

		Transform(const glm::vec3& t, const glm::vec3& r, const glm::vec3 s) : translation(t), rotation(glm::quat(r)), scale(s) { recalculate(); }
		
		void recalculate()
		{
			glm::mat4 t = glm::translate(glm::mat4(1.f), translation);
			glm::mat4 r = glm::toMat4(rotation);
			glm::mat4 s = glm::scale(glm::mat4(1.f), scale);

			transform = t * r * s;
		}
	};
}
