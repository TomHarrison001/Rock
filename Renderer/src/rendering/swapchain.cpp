/** \file swapchain.cpp */

#include "rendering/swapchain.hpp"

Swapchain::Swapchain(Device* device, VkSampleCountFlagBits msaaSamples, bool resources) : m_device(device), m_resources(resources)
{
    createSwapchain();
    createImageViews();
    if (m_resources)
        createResources(msaaSamples);
    createRenderPass(msaaSamples);
    createFramebuffers();
    createSyncObjects();
}

Swapchain::Swapchain(Device* device, Swapchain* oldSwapchain, VkSampleCountFlagBits msaaSamples) : m_device(device)
{
    m_resources = oldSwapchain->getResources();
    createSwapchain(oldSwapchain->getSwapchain());
    createImageViews();
    if (m_resources)
        createResources(msaaSamples);
    createRenderPass(msaaSamples);
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
    if (m_resources)
    {
        vkDestroyImageView(m_device->getDevice(), m_depthImageView, nullptr);
        vkDestroyImage(m_device->getDevice(), m_depthImage, nullptr);
        vkFreeMemory(m_device->getDevice(), m_depthImageMemory, nullptr);
        vkDestroyImageView(m_device->getDevice(), m_colourImageView, nullptr);
        vkDestroyImage(m_device->getDevice(), m_colourImage, nullptr);
        vkFreeMemory(m_device->getDevice(), m_colourImageMemory, nullptr);
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
        vkDestroyFence(m_device->getDevice(), m_fences[i], nullptr);
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
        m_swapchainImageViews[i] = m_device->createImageView(m_swapchainImages[i], m_swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void Swapchain::createResources(VkSampleCountFlagBits msaaSamples)
{
    VkFormat format = m_swapchainImageFormat;
    m_device->createImage(m_swapchainExtent.width, m_swapchainExtent.height, 1, msaaSamples, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colourImage, m_colourImageMemory);
    m_colourImageView = m_device->createImageView(m_colourImage, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    format = m_swapchainDepthFormat;
    m_device->createImage(m_swapchainExtent.width, m_swapchainExtent.height, 1, msaaSamples, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
    m_depthImageView = m_device->createImageView(m_depthImage, format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void Swapchain::createRenderPass(VkSampleCountFlagBits msaaSamples)
{
    VkAttachmentDescription colourAttachment{};
    colourAttachment.format = m_swapchainImageFormat;
    colourAttachment.samples = msaaSamples;
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

    std::vector<VkAttachmentDescription> attachments = { colourAttachment };

    if (m_resources)
    {
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = m_swapchainDepthFormat;
        depthAttachment.samples = msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments.push_back(depthAttachment);

        VkAttachmentDescription colourAttachmentResolve{};
        colourAttachmentResolve.format = m_swapchainImageFormat;
        colourAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colourAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colourAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colourAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colourAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colourAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colourAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        attachments.push_back(colourAttachmentResolve);

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colourAttachmentResolveRef{};
        colourAttachmentResolveRef.attachment = 2;
        colourAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colourAttachmentResolveRef;

        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
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
        std::vector<VkImageView> attachments = {};
        if (m_resources) {
            attachments.push_back(m_colourImageView);
            attachments.push_back(m_depthImageView);
        }
        attachments.push_back(m_swapchainImageViews[i]);

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
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
    m_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_graphicsFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device->getDevice(), &fenceInfo, nullptr, &m_fences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create synchronisation objects for a frame.");
    }
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
#ifdef FRAMERATE_CAP
    return VK_PRESENT_MODE_FIFO_KHR;
#endif
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
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
