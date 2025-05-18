/** \file window.cpp */

#include "window/window.hpp"

Window::Window()
{
	m_settings = WindowSettings();
	this->initWindow();
}

Window::Window(WindowSettings settings)
	: m_settings(settings)
{
	this->initWindow();
}

Window::~Window()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Window::initWindow()
{
	if (!glfwInit())
		return;

	/* removes default opengl api */
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	/* Sets window resizable */
	glfwWindowHint(GLFW_RESIZABLE, m_settings.resizable ? GLFW_TRUE : GLFW_FALSE);

	m_window = glfwCreateWindow(m_settings.width, m_settings.height, m_settings.title, nullptr, nullptr);
	if (!glfwVulkanSupported)
	{
		std::cout << "[GLFW] Vulkan not supported." << std::endl;
		return;
	}
	
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto winRef = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	winRef->m_resized = true;
	winRef->m_settings.width = width;
	winRef->m_settings.height = height;
	winRef->m_settings.aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

void Window::createSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS)
		throw std::runtime_error("Failed to create window surface.");
}
