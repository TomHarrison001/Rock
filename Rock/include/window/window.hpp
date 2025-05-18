/** \file window.hpp */

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

/* \struct WindowSettings
*  \brief stores the settings of a window
*/
struct WindowSettings
{
	const char* title{ "Rock" }; //!< title on menu bar
	uint32_t width{ 720 }; //!< width in pixels
	uint32_t height{ 480 }; //!< height in pixels
	float aspectRatio{ static_cast<float>(width) / static_cast<float>(height) }; //!< window aspect ratio
	bool fullscreen{ false }; //!< is window fullscreen
	bool resizable{ true }; //!< can window be resized
	bool isVsync{ false }; //!< is vsync enabled
	bool imguiEnabled{ false }; //!< does the window host imgui

	WindowSettings() { } //!< default constructor
};

/* \class Window
*  \brief stores the GLFWwindow* and WindowSettings, handles resizing
*/
class Window
{
public:
	Window(); //!< default constructor
	Window(WindowSettings settings); //!< constructor
	~Window(); //!< destructor
	Window(const Window&) = delete; //!< copy constructor
	Window(const Window&&) = delete; //!< move constructor
	Window& operator=(const Window&) = delete; //!< copy assignment
	Window& operator=(const Window&&) = delete; //!< move assignment
private:
	void initWindow(); //!< create and initialise glfw window
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height); //!< function when window is resized
public:
	bool shouldClose() { return glfwWindowShouldClose(m_window); } //!< returns if the window should close
	void createSurface(VkInstance instance, VkSurfaceKHR* surface); //!< creates surface and stores as VkSurfaceKHR
	GLFWwindow* getWindow() { return m_window; } //!< get pointer to glfw window
	uint32_t getWidth() const { return m_settings.width; } //!< get window width from settings
	uint32_t getHeight() const { return m_settings.height; } //!< get window height from settings
	bool getResized() const { return m_resized; } //!< returns bool of if the window was resized
	void setResized(bool resized) { m_resized = resized; } //!< returns bool of if the window was resized
	float getAspectRatio() const { return m_settings.aspectRatio; } //!< get window aspect ratio from settings
	VkExtent2D getExtent() { return { static_cast<uint32_t>(m_settings.width), static_cast<uint32_t>(m_settings.height) }; } //!< returns the actual extent of the window
private:
	GLFWwindow* m_window; //!< glfw window
	WindowSettings m_settings; //!< window settings
	bool m_resized = false; //!< true after window gets resized
};
