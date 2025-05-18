/** \file eventSystem.cpp */

#include "window/eventSystem.hpp"

bool EventSystem::isKeyPressed(GLFWwindow* window, int keycode)
{
	auto answer = glfwGetKey(window, keycode);
	return answer == GLFW_PRESS || answer == GLFW_REPEAT;
}

bool EventSystem::isMouseButtonPressed(GLFWwindow* window, int mouseBtn)
{
	auto answer = glfwGetMouseButton(window, mouseBtn);
	return answer == GLFW_PRESS;
}

glm::vec2 EventSystem::getMousePosition(GLFWwindow* window)
{
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	return glm::vec2(static_cast<float>(x), static_cast<float>(y));
}
