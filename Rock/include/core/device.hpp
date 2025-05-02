#pragma once

#include "window/window.hpp"

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
    std::optional<uint32_t> graphicsFamily; // index of physical device queue family supporting VK_QUEUE_GRAPHICS_BIT and VK_QUEUE_COMPUTE_BIT
    std::optional<uint32_t> presentFamily; // index of physical device queue family supporting present
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

    Device(Window* window); // constructor
    ~Device(); // destructor
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
    Window* getWindow() { return m_window; } //!< returns the window object reference
    VkSurfaceKHR getSurface() { return m_surface; } //!< returns the surface
    VkDevice getDevice() { return m_device; } //!< returns the device
    VkQueue getRenderQueue() { return m_renderQueue; } //!< returns the render queue
    VkQueue getComputeQueue() { return m_computeQueue; } //!< returns the compute queue
    VkQueue getPresentQueue() { return m_presentQueue; } //!< returns the present queue
    VkCommandPool getCommandPool() { return m_commandPool; } //!< returns the command pool

    SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(m_physicalDevice); } //!< returns the swap chain support
    QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(m_physicalDevice); } //!< returns the queue families
private:
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
private:
    Window* m_window; //!< window object pointer
    VkInstance m_instance; //!< vulkan instance
    VkDebugUtilsMessengerEXT m_debugMessenger; //!< debug utils messenger ext
    VkSurfaceKHR m_surface; //!< device surface
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE; //!< physical device
    VkDevice m_device; //!< device
    VkQueue m_renderQueue; //!< render queue
    VkQueue m_computeQueue; //!< compute queue
    VkQueue m_presentQueue; //!< present queue
    VkCommandPool m_commandPool; //!< command pool
};
