/** \file eventSystem.hpp */

#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

/* \class EventSystem
*  \brief provides an interface using glfw for keyboard and mouse inputs
*/
class EventSystem
{
public:
	static bool isKeyPressed(GLFWwindow* window, int keycode); //!< checks if the input keycode is currently being pressed
	static bool isMouseButtonPressed(GLFWwindow* window, int mouseBtn); //!< checks if the input mouse button is currently being pressed
	static glm::vec2 getMousePosition(GLFWwindow* window); //!< returns the current mouse position
};
