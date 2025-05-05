/** \file swapchain.cpp */

#include "rendering/swapchain.hpp"

Swapchain::Swapchain(Device* device) : m_device(device)
{
    createSwapchain();
    createImageViews();
    //createResources();
    createRenderPass();
    createFramebuffers();
    createSyncObjects();
}

Swapchain::Swapchain(Device* device, Swapchain* oldSwapchain) : m_device(device)
{
    createSwapchain(oldSwapchain->getSwapchain());
    createImageViews();
    //createResources();
    createRenderPass();
    createFramebuffers();
    createSyncObjects();
}

Swapchain::~Swapchain()
{
    if (m_swapchain != nullptr)
    {
        vkDestroySwapchainKHR(m_device->getDevice(), m_swapchain, nullptr);
        m_swapchain = nullptr;
    }
    for (int i = 0; i < m_swapchainImages.size(); i++)
    {
        vkDestroyImageView(m_device->getDevice(), m_swapchainImageViews[i], nullptr);
        //vkDestroyImage(m_device->getDevice(), m_swapchainImages[i], nullptr);
        vkFreeMemory(m_device->getDevice(), m_swapchainImageMemory[i], nullptr);
    }
    m_swapchainImages.clear();
    for (auto framebuffer : m_swapchainFramebuffers)
    {
        vkDestroyFramebuffer(m_device->getDevice(), framebuffer, nullptr);
    }
    m_swapchainFramebuffers.clear();
    vkDestroyRenderPass(m_device->getDevice(), m_renderPass, nullptr);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_device->getDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_device->getDevice(), m_graphicsFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_device->getDevice(), m_computeFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_device->getDevice(), m_graphicsInFlightFences[i], nullptr);
        vkDestroyFence(m_device->getDevice(), m_computeInFlightFences[i], nullptr);
    }
    m_device = nullptr;
}

void Swapchain::createSwapchain(VkSwapchainKHR oldSwapchain)
{
    SwapChainSupportDetails swapchainSupport = m_device->getSwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = choosePresentMode(swapchainSupport.presentModes);
    VkExtent2D extent = chooseExtent(swapchainSupport.capabilities);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
        imageCount = swapchainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = m_device->getSurface();
    ci.minImageCount = imageCount;
    ci.imageFormat = surfaceFormat.format;
    ci.imageColorSpace = surfaceFormat.colorSpace;
    ci.imageExtent = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // VK_IMAGE_USAGE_TRANSFER_DST_BIT to render images to a separate image first; e.g. post-processing
    ci.oldSwapchain = oldSwapchain;

    QueueFamilyIndices indices = m_device->findPhysicalQueueFamilies();
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // images can be used across multiple queue families without explicit ownership transfers
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // an image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family
        ci.queueFamilyIndexCount = 0; // optional
        ci.pQueueFamilyIndices = nullptr; // optional
    }

    ci.preTransform = swapchainSupport.capabilities.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = presentMode;
    ci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_device->getDevice(), &ci, nullptr, &m_swapchain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain.");

    vkGetSwapchainImagesKHR(m_device->getDevice(), m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device->getDevice(), m_swapchain, &imageCount, m_swapchainImages.data());

    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;
    m_swapchainDepthFormat = findDepthFormat();
}

void Swapchain::createImageViews()
{
    m_swapchainImageViews.resize(m_swapchainImages.size());
    m_swapchainImageMemory.resize(m_swapchainImages.size());

    for (size_t i = 0; i < m_swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.image = m_swapchainImages[i];
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ci.format = m_swapchainImageFormat;
        ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ci.subresourceRange.baseMipLevel = 0;
        ci.subresourceRange.levelCount = 1;
        ci.subresourceRange.baseArrayLayer = 0;
        ci.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device->getDevice(), &ci, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image views.");
    }
}

void Swapchain::createResources(Resource type)
{
    VkFormat format = (type == Resource::COLOUR) ? m_swapchainImageFormat : m_swapchainDepthFormat;
    VkImageUsageFlags usage = (type == Resource::COLOUR) ?
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT :
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageAspectFlags flags = (type == Resource::COLOUR) ?
        VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;

    for (int i = 0; i < m_swapchainImages.size(); i++)
    {
        m_device->createImage(m_swapchainExtent.width, m_swapchainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_swapchainImages[i], m_swapchainImageMemory[i]);
        m_swapchainImageViews[i] = createImageView(m_swapchainImages[i], format, flags, 1);
    }
}

void Swapchain::createRenderPass()
{
    VkAttachmentDescription colourAttachment{};
    colourAttachment.format = m_swapchainImageFormat;
    colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colourAttachmentRef{};
    colourAttachmentRef.attachment = 0;
    colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colourAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device->getDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass.");
}

void Swapchain::createFramebuffers()
{
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

    for (size_t i = 0; i < m_swapchainImageViews.size(); i++)
    {
        VkImageView attachments[] = {
            m_swapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device->getDevice(), &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer.");
    }
}

void Swapchain::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_graphicsFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_graphicsInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_graphicsFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device->getDevice(), &fenceInfo, nullptr, &m_graphicsInFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render synchronisation objects for a frame.");
        if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_computeFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device->getDevice(), &fenceInfo, nullptr, &m_computeInFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute synchronisation objects for a frame.");
    }
}

VkImageView Swapchain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.image = image;
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ci.format = format;
    ci.subresourceRange.aspectMask = aspectFlags;
    ci.subresourceRange.baseMipLevel = 0;
    ci.subresourceRange.levelCount = mipLevels;
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(m_device->getDevice(), &ci, nullptr, &imageView) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image view.");

    return imageView;
}

VkSurfaceFormatKHR Swapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    return availableFormats[0];
}

VkPresentModeKHR Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    VkExtent2D actualExtent = m_device->getWindow()->getExtent();

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

VkFormat Swapchain::findDepthFormat()
{
    return m_device->findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkResult Swapchain::acquireNextImage(uint32_t* imageIndex)
{
    return vkAcquireNextImageKHR(m_device->getDevice(), m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, imageIndex);
}

VkResult Swapchain::submitCommandBuffers(VkCommandBuffer commandBuffer, uint32_t* imageIndex)
{
    VkSemaphore waitSemaphores[] = { m_computeFinishedSemaphores[m_currentFrame], m_imageAvailableSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { m_graphicsFinishedSemaphores[m_currentFrame] };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 2;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_device->getRenderQueue(), 1, &submitInfo, m_graphicsInFlightFences[m_currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer.");

    VkSwapchainKHR swapchains[] = { m_swapchain };
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = imageIndex;
    presentInfo.pResults = nullptr; // optional

    VkResult result = vkQueuePresentKHR(m_device->getPresentQueue(), &presentInfo);

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}
