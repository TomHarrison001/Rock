/** \file swapchain.hpp */

#pragma once

#include "core/device.hpp"

#include <vulkan/vulkan.h>
#include <memory>
#include <algorithm>

//#define FRAMERATE_CAP

/* \class Swapchain
*  \brief handles the swapchain lifecycle including images, image views, image memory, framebuffers and render passes
*/
class Swapchain
{
public:
	static const int MAX_FRAMES_IN_FLIGHT = 2; //!< static var containing maxmimum number of potential frames in flight
	Swapchain(Device* device, VkSampleCountFlagBits msaaSamples, bool resources); //!< constructor
	Swapchain(Device* device, Swapchain* oldSwapchain, VkSampleCountFlagBits msaaSamples); //!< constructor with additional input of previous swapchain
	~Swapchain(); //!< destructor
    Swapchain(const Swapchain&) = delete; //!< copy constructor
    Swapchain& operator=(const Swapchain&) = delete; //!< copy assignment

    VkSwapchainKHR getSwapchain() const { return m_swapchain; } //!< returns the VkSwapchainKHR object
    VkFormat getSwapchainImageFormat() const { return m_swapchainImageFormat; } //!< used to check that new/old swapchains have the same image format
    VkFormat getSwapchainDepthFormat() const { return m_swapchainDepthFormat; } //!< used to check that new/old swapchains have the same depth format
    VkExtent2D getSwapchainExtent() const { return m_swapchainExtent; } //!< returns the swapchain extent (could be different to window due to surface capabilities)
    VkRenderPass getRenderPass() const { return m_renderPass; } //!< returns render pass for the pipeline
    uint32_t getImageCount() const { return m_swapchainImages.size(); } //!< returns the number of images in swapchain images
    VkFramebuffer getFramebuffer(int index) { return m_swapchainFramebuffers[index]; } //!< returns the framebuffer for the command buffer
    VkSemaphore& getImageAvailableSemaphore(uint32_t currentFrame) { return m_imageAvailableSemaphores[currentFrame]; } //!< image available semaphore getter method for current frame
    VkSemaphore& getGraphicsFinishedSemaphore(uint32_t currentFrame) { return m_graphicsFinishedSemaphores[currentFrame]; } //!< graphics finished semaphore getter method for current frame
    VkFence& getFence(uint32_t currentFrame) { return m_fences[currentFrame]; } //!< in flight fence getter method for current frame
    bool getResources() const { return m_resources; } //!< returns if the swapchain should create images, image views and image memory for colour and depth
    bool operator!=(const Swapchain* swapchain) const {
        return swapchain->getSwapchainImageFormat() != m_swapchainImageFormat ||
            swapchain->getSwapchainDepthFormat() != m_swapchainDepthFormat;
    } //!< used to check if old/new swapchain support the same image and depth formats
private:
    void createSwapchain(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE); //!< defines and initialises the swapchain
    void createImageViews(); //!< creates image views
    void createResources(VkSampleCountFlagBits msaaSamples); //!< creates resources for colour and depth
    void createRenderPass(VkSampleCountFlagBits msaaSamples); //!< creates render pass
    void createFramebuffers(); //!< initialises framebuffer
    void createSyncObjects(); //!< initialises sync objects (semaphores and fences)

    VkFormat findDepthFormat(); //!< finds device supported depth format
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats); //!< chooses best available surface format
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes); //!< chooses best available present mode
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities); //!< determines swapchain extent based on capabilities and window extent

    Device* m_device; //!< device object pointer
    VkSwapchainKHR m_swapchain; //!< swapchain object
    VkFormat m_swapchainImageFormat; //!< image format
    VkFormat m_swapchainDepthFormat; //!< depth format
    VkExtent2D m_swapchainExtent; //!< extent of the swapchain
    VkRenderPass m_renderPass; //!< render pass
    std::vector<VkImage> m_swapchainImages; //!< vector of swapchain images
    std::vector<VkImageView> m_swapchainImageViews; //!< vector of swapchain image views
    std::vector<VkDeviceMemory> m_swapchainImageMemory; //!< vector of swapchain image memory
    
    std::vector<VkFramebuffer> m_swapchainFramebuffers; //!< swapchain framebuffers
    std::vector<VkSemaphore> m_imageAvailableSemaphores; //!< semaphores for image availability
    std::vector<VkSemaphore> m_graphicsFinishedSemaphores; //!< semaphores for graphics shaders
    std::vector<VkFence> m_fences; //!< in flight fences
    
    VkImage m_colourImage; //!< image for colour image resource
    VkDeviceMemory m_colourImageMemory; //!< memory for colour image resource
    VkImageView m_colourImageView; //!< image view for colour image resource
    VkImage m_depthImage; //!< image for depth image resource
    VkDeviceMemory m_depthImageMemory; //!< memory for depth image resource
    VkImageView m_depthImageView; //!< image view for depth image resource

    bool m_resources; //!< bool if the swapchain should create images, image views and image memory for colour and depth
};
