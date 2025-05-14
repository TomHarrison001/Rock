/** \file device.hpp */

#pragma once

#include "window/window.hpp"
#include "window/eventSystem.hpp"

#include <iostream>
#include <vector>
#include <optional>
#include <set>

/* \struct SwapChainSupportDetails
*  \brief stores the capabilities, formats and present modes for the device
*/
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities; //!< capabilities of the surface
    std::vector<VkSurfaceFormatKHR> formats; //!< supported swapchain format-colour space pair
    std::vector<VkPresentModeKHR> presentModes; //!< presentation mode supported for a surface
};

/* \struct QueueFamilyIndices
*  \brief stores optional values to check both queue families
*/
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily; //!< index of physical device queue family supporting VK_QUEUE_GRAPHICS_BIT and VK_QUEUE_COMPUTE_BIT
    std::optional<uint32_t> presentFamily; //!< index of physical device queue family supporting present
    bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); } //!< returns if physical device supports graphics, compute and present
};

/* \class Device
*  \brief stores the window, surface, instance, debug messenger, command pools, etc.
*/
class Device
{
public:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    Device(); //!< default constructor
    Device(Window* window); //!< constructor
    ~Device(); //!< destructor
    Device(const Device&) = delete; //!< copy constructor
    Device(const Device&&) = delete; //!< move constructor
    Device& operator=(const Device&) = delete; //!< copy assignment
    Device& operator=(const Device&&) = delete; //!< move assignment
private:
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
public:
    Window* getWindow() const { return m_window; } //!< returns the window object
    VkSurfaceKHR getSurface() const { return m_surface; } //!< returns the surface
    VkDevice getDevice() const { return m_device; } //!< returns the device
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; } //!< returns the device
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; } //!< returns the graphics queue
    VkQueue getComputeQueue() const { return m_computeQueue; } //!< returns the compute queue
    VkQueue getPresentQueue() const { return m_presentQueue; } //!< returns the present queue
    VkCommandPool getCommandPool() const { return m_commandPool; } //!< returns the command pool

    SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(m_physicalDevice); } //!< returns the swap chain support
    QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(m_physicalDevice); } //!< returns the queue families

    bool isKeyPressed(int keycode) { return EventSystem::isKeyPressed(m_window->getWindow(), keycode); } //!< checks if the input keycode is currently being pressed
    bool isMouseButtonPressed(int mouseBtn) { return EventSystem::isMouseButtonPressed(m_window->getWindow(), mouseBtn); } //!< checks if the input mouse button is currently being pressed
    glm::vec2 getMousePosition() { return EventSystem::getMousePosition(m_window->getWindow()); } //!< returns the current mouse position
    void closeWindow() { glfwSetWindowShouldClose(m_window->getWindow(), true); } //!< closes the GLFW window
private:
    void initDevice(); //!< initialises the device
    void createInstance(); //!< creates the instance
    void setupDebugMessenger(); //!< performs the setup required for the debug utils messenger EXT
    void createSurface(); //!< creates the surface using the window
    void pickPhysicalDevice(); //!< chooses the most suitable GPU with vulkan support
    void createLogicalDevice(); //!< creates the device using the physical device and gets device queues
    void createCommandPool(); //!< creates the command pool

    bool checkValidationLayerSupport(); //!< checks the validation layer(s) is supported
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& ci); //!< populates the create info for the debug messenger
    bool isDeviceSuitable(VkPhysicalDevice device); //!< checks if the input device supports the required extensions and swapchains
    std::vector<const char*> getRequiredExtensions(); //!< returns the required extensions
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device); //!< returns the indices of physical device queue families
    bool checkDeviceExtensionSupport(VkPhysicalDevice device); //!< checks the device supports all the required extensions
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device); //!< gets swap chain support details for input device
public:
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties); //!< finds memory index match input properties and filters
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory); //!< creates buffer, allocates memory and binds with device
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size); //!< copies buffer; used for creating staged buffers before copying to buffer array (e.g. ssbos)
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features); //!< finds supported format favouring VK_IMAGE_TILING_LINEAR
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory); //!< creates an image and binds ith with the device to device memory
private:
    Window* m_window; //!< window object pointer
    VkInstance m_instance; //!< vulkan instance
    VkDebugUtilsMessengerEXT m_debugMessenger; //!< debug utils messenger ext
    VkSurfaceKHR m_surface; //!< device surface
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE; //!< physical device
    VkDevice m_device; //!< device
    VkQueue m_graphicsQueue; //!< graphics queue
    VkQueue m_computeQueue; //!< compute queue
    VkQueue m_presentQueue; //!< present queue
    VkCommandPool m_commandPool; //!< command pool
};
