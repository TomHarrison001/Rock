/** \file lights.hpp */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

/* \struct DirectionalLight
*  \brief stores the data for a directional light
*/
struct DirectionalLight
{
	glm::vec3 colour{ 1.f }; //!< colour of the light
	glm::vec3 direction{ 0.f, -1.f, 0.f }; //!< direction the light is pointing
};

/* \struct PointLight
*  \brief stores the data for a point light
*/
struct PointLight
{
	glm::vec3 colour{ 1.f }; //!< colour of the light
	glm::vec3 position{ 0.f }; //!< position of the light
	glm::vec3 constants{ 1.f, 0.1f, 0.01f }; //!< constant, linear, exponential
};

/* \struct SpotLight
*  \brief stores the data for a spot light
*/
struct SpotLight
{
	glm::vec3 colour{ 1.f }; //!< colour of the light
	glm::vec3 position{ 0.f }; //!< position of the light
	glm::vec3 direction{ 0.f, 0.f, -1.f }; //!< direction the light is pointing
	glm::vec3 constants{ 1.f, 0.1f, 0.01f }; //!< constant, linear, exponential
	float innerCutOffCosine = cosf(glm::quarter_pi<float>()); //!< inner cut off angle, start of fade
	float outerCutOffCosine = cosf(glm::half_pi<float>()); //!< outer cut off angle, end of fade
};
