#pragma once

#include "core/device.hpp"
#include "utils/resources.hpp"

#include <vulkan/vulkan.h>
#include <memory>
#include <algorithm>

/* \class Swapchain
*  \brief handles the swapchain lifecycle including images, image views, image memory, framebuffers and render passes
*/
class Swapchain
{
public:
	static const int MAX_FRAMES_IN_FLIGHT = 2; //!< static var containing maxmimum number of potential frames in flight
	Swapchain(Device* device); //!< constructor
	Swapchain(Device* device, Swapchain* oldSwapchain); //!< constructor with additional input of previous swapchain
	~Swapchain(); //!< destructor
    Swapchain(const Swapchain&) = delete; //!< copy constructor
    Swapchain& operator=(const Swapchain&) = delete; //!< copy assignment

    VkSwapchainKHR getSwapchain() const { return m_swapchain; } //!< returns the VkSwapchainKHR object
    VkFormat getSwapchainImageFormat() const { return m_swapchainImageFormat; } //!< used to check that new/old swapchains have the same image format
    VkFormat getSwapchainDepthFormat() const { return m_swapchainDepthFormat; } //!< used to check that new/old swapchains have the same depth format
    VkExtent2D getSwapchainExtent() const { return m_swapchainExtent; } //!< returns the swapchain extent (could be different to window due to surface capabilities)
    VkRenderPass getRenderPass() const { return m_renderPass; } //!< returns render pass for the pipeline
    VkFramebuffer getFramebuffer(int index) { return m_swapchainFramebuffers[index]; } //!< returns the framebuffer for the command buffer
    VkSemaphore& getImageAvailableSemaphore(uint32_t currentFrame) { return m_imageAvailableSemaphores[currentFrame]; } //!< image available semaphore getter method for current frame
    VkSemaphore& getGraphicsFinishedSemaphore(uint32_t currentFrame) { return m_graphicsFinishedSemaphores[currentFrame]; } //!< graphics finished semaphore getter method for current frame
    VkSemaphore& getComputeFinishedSemaphore(uint32_t currentFrame) { return m_computeFinishedSemaphores[currentFrame]; } //!< compute finished semaphore getter method for current frame
    VkFence& getGraphicsInFlightFence(uint32_t currentFrame) { return m_graphicsInFlightFences[currentFrame]; } //!< graphics in flight fence getter method for current frame
    VkFence& getComputeInFlightFence(uint32_t currentFrame) { return m_computeInFlightFences[currentFrame]; } //!< compute in flight fence getter method for current frame
    bool operator!=(const Swapchain* swapchain) const {
        return swapchain->getSwapchainImageFormat() != m_swapchainImageFormat ||
            swapchain->getSwapchainDepthFormat() != m_swapchainDepthFormat;
    } //!< used to check if old/new swapchain support the same image and depth formats
private:
    void createSwapchain(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE); //!< defines and initialises the swapchain
    void createImageViews(); //!< creates image views
    void createResources(Resource type); //!< creates resources (colour or depth, see resourceType)
    void createRenderPass(); //!< creates render pass
    void createFramebuffers(); //!< initialises framebuffer
    void createSyncObjects(); //!< initialises sync objects (semaphores and fences)

    VkFormat findDepthFormat(); //!< finds device supported depth format
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels); //!< creates an image view
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
    std::vector<VkSemaphore> m_computeFinishedSemaphores; //!< semaphores for compute shaders
    std::vector<VkFence> m_graphicsInFlightFences; //!< fence for graphics
    std::vector<VkFence> m_computeInFlightFences; //!< fence for compute
};
