#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <limits>
#include <algorithm>
#include <array>
#include <chrono>
#include <random>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/device.hpp"

const int MAX_FRAMES_IN_FLIGHT = 2;

struct Particle
{
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 colour;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Particle, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Particle, colour);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    float u_deltaTime = 1.f;
};

class Application
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initWindow()
    {
        Window* window = new Window();
        m_device = new Device(window);
        window = nullptr;
        m_lastTime = glfwGetTime();
    }

    void initVulkan()
    {
        createSwapChain();
        createImageViews();
        createRenderPass();
        createComputeDescriptorSetLayout();
        createRenderPipeline();
        createComputePipeline();
        createFramebuffers();
        createShaderStorageBuffers();
        createUniformBuffers();
        createDescriptorPool();
        createComputeDescriptorSets();
        createRenderCommandBuffers();
        createComputeCommandBuffers();
        createSyncObjects();
    }

    void mainLoop()
    {
        while (!m_device->getWindow()->shouldClose())
        {
            glfwPollEvents();
            drawFrame();
            double currentTime = glfwGetTime();
            m_lastFrameTime = (currentTime - m_lastTime) * 1000.f;
            m_lastTime = currentTime;
        }

        vkDeviceWaitIdle(m_device->getDevice());
    }

    void cleanup()
    {
        cleanupSwapChain();
        vkDestroyPipeline(m_device->getDevice(), m_renderPipeline, nullptr);
        vkDestroyPipelineLayout(m_device->getDevice(), m_renderPipelineLayout, nullptr);
        vkDestroyPipeline(m_device->getDevice(), m_computePipeline, nullptr);
        vkDestroyPipelineLayout(m_device->getDevice(), m_computePipelineLayout, nullptr);
        vkDestroyRenderPass(m_device->getDevice(), m_renderPass, nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(m_device->getDevice(), m_uniformBuffers[i], nullptr);
            vkFreeMemory(m_device->getDevice(), m_uniformBuffersMemory[i], nullptr);
        }
        vkDestroyDescriptorPool(m_device->getDevice(), m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device->getDevice(), m_computeDescriptorSetLayout, nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(m_device->getDevice(), m_shaderStorageBuffers[i], nullptr);
            vkFreeMemory(m_device->getDevice(), m_shaderStorageBuffersMemory[i], nullptr);
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(m_device->getDevice(), m_imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_device->getDevice(), m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_device->getDevice(), m_computeFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_device->getDevice(), m_renderInFlightFences[i], nullptr);
            vkDestroyFence(m_device->getDevice(), m_computeInFlightFences[i], nullptr);
        }
    }

    void createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = m_device->getSwapChainSupport();

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
            imageCount = swapChainSupport.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR ci{};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface = m_device->getSurface();
        ci.minImageCount = imageCount;
        ci.imageFormat = surfaceFormat.format;
        ci.imageColorSpace = surfaceFormat.colorSpace;
        ci.imageExtent = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // VK_IMAGE_USAGE_TRANSFER_DST_BIT to render images to a separate image first; e.g. post-processing

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

        ci.preTransform = swapChainSupport.capabilities.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode = presentMode;
        ci.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(m_device->getDevice(), &ci, nullptr, &m_swapChain) != VK_SUCCESS)
            throw std::runtime_error("Failed to create swap chain.");

        vkGetSwapchainImagesKHR(m_device->getDevice(), m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device->getDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    void recreateSwapChain()
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_device->getWindow()->getWindow(), &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(m_device->getWindow()->getWindow(), &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_device->getDevice());

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createFramebuffers();
    }

    void cleanupSwapChain()
    {
        for (auto framebuffer : m_swapChainFramebuffers)
        {
            vkDestroyFramebuffer(m_device->getDevice(), framebuffer, nullptr);
        }
        for (auto imageView : m_swapChainImageViews)
        {
            vkDestroyImageView(m_device->getDevice(), imageView, nullptr);
        }
        vkDestroySwapchainKHR(m_device->getDevice(), m_swapChain, nullptr);
    }

    void createImageViews()
    {
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.image = m_swapChainImages[i];
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = m_swapChainImageFormat;
            ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ci.subresourceRange.baseMipLevel = 0;
            ci.subresourceRange.levelCount = 1;
            ci.subresourceRange.baseArrayLayer = 0;
            ci.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device->getDevice(), &ci, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create image views.");
        }
    }

    void createRenderPass()
    {
        VkAttachmentDescription colourAttachment{};
        colourAttachment.format = m_swapChainImageFormat;
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

    void createRenderDescriptorSetLayout()
    {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].pImmutableSamplers = nullptr; // optional

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(m_device->getDevice(), &layoutInfo, nullptr, &m_renderDescriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render descriptor set layout.");
    }

    void createComputeDescriptorSetLayout()
    {
        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[0].pImmutableSamplers = nullptr; // optional

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        bindings[2].binding = 2;
        bindings[2].descriptorCount = 1;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[2].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(m_device->getDevice(), &layoutInfo, nullptr, &m_computeDescriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute descriptor set layout.");
    }

    void createRenderPipeline()
    {
        auto vertShaderCode = readFile("./res/shaders/vert.spv");
        auto fragShaderCode = readFile("./res/shaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main"; // function to invoke; i.e. entrypoint

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main"; // function to invoke; i.e. entrypoint

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = Particle::getBindingDescription();
        auto attributeDescriptions = Particle::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasteriser{};
        rasteriser.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasteriser.depthClampEnable = VK_FALSE;
        rasteriser.rasterizerDiscardEnable = VK_FALSE;
        rasteriser.polygonMode = VK_POLYGON_MODE_FILL; // fill the area of the polygon with fragments
        rasteriser.lineWidth = 1.f;
        rasteriser.cullMode = VK_CULL_MODE_BACK_BIT;
        rasteriser.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // counter clockwise due to the Y-flip in the projection matrix
        rasteriser.depthBiasEnable = VK_FALSE;
        rasteriser.depthBiasConstantFactor = 0.f; // optional
        rasteriser.depthBiasClamp = 0.f; // optional
        rasteriser.depthBiasSlopeFactor = 0.f; // optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.f; // optional
        multisampling.pSampleMask = nullptr; // optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // optional
        multisampling.alphaToOneEnable = VK_FALSE; // optional

        VkPipelineColorBlendAttachmentState colourBlendAttachment{};
        colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colourBlendAttachment.blendEnable = VK_TRUE;
        colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        VkPipelineColorBlendStateCreateInfo colourBlending{};
        colourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colourBlending.logicOpEnable = VK_FALSE; // VK_TRUE for alpha blending
        colourBlending.logicOp = VK_LOGIC_OP_COPY; // optional; specify bitwise operation for alpha blending
        colourBlending.attachmentCount = 1;
        colourBlending.pAttachments = &colourBlendAttachment;
        colourBlending.blendConstants[0] = 0.f; // optional
        colourBlending.blendConstants[1] = 0.f; // optional
        colourBlending.blendConstants[2] = 0.f; // optional
        colourBlending.blendConstants[3] = 0.f; // optional

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // optional

        if (vkCreatePipelineLayout(m_device->getDevice(), &pipelineLayoutInfo, nullptr, &m_renderPipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pipeline layout.");

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasteriser;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colourBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_renderPipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1; // optional

        if (vkCreateGraphicsPipelines(m_device->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_renderPipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pipeline.");

        vkDestroyShaderModule(m_device->getDevice(), vertShaderModule, nullptr);
        vkDestroyShaderModule(m_device->getDevice(), fragShaderModule, nullptr);
    }

    void createComputePipeline()
    {
        auto compShaderCode = readFile("./res/shaders/comp.spv");

        VkShaderModule compShaderModule = createShaderModule(compShaderCode);

        VkPipelineShaderStageCreateInfo compShaderStageInfo{};
        compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        compShaderStageInfo.module = compShaderModule;
        compShaderStageInfo.pName = "main";

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_computeDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // optional

        if (vkCreatePipelineLayout(m_device->getDevice(), &pipelineLayoutInfo, nullptr, &m_computePipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute pipeline layout.");

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = m_computePipelineLayout;
        pipelineInfo.stage = compShaderStageInfo;

        if (vkCreateComputePipelines(m_device->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_computePipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute pipeline.");

        vkDestroyShaderModule(m_device->getDevice(), compShaderModule, nullptr);
    }

    void createFramebuffers()
    {
        m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
        
        for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = {
                m_swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_swapChainExtent.width;
            framebufferInfo.height = m_swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device->getDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer.");
        }
    }

    void createShaderStorageBuffers()
    {
        // initialise particles
        std::default_random_engine rndEngine((unsigned)time(nullptr));
        std::uniform_real_distribution<float> rndDist(0.f, 1.f);

        // initialise particle positions on a circle
        std::vector<Particle> particles(m_particleCount);
        for (auto& particle : particles)
        {
            float r = 0.25f * sqrt(rndDist(rndEngine));
            float theta = rndDist(rndEngine) * 2.f * 3.1415926f;
            //float x = r * cos(theta) / m_window->getAspectRatio();
            float x = r * cos(theta);
            //float y = r * sin(theta) * m_window->getAspectRatio();
            float y = r * sin(theta);
            particle.position = glm::vec2(x, y);
            particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
            particle.colour = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.f);
        }

        VkDeviceSize bufferSize = sizeof(Particle) * m_particleCount;

        // create a staging buffer to upload ssbo data
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(m_device->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, particles.data(), (size_t)bufferSize);
        vkUnmapMemory(m_device->getDevice(), stagingBufferMemory);

        m_shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

        // copy initial particle data to storage buffer
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_shaderStorageBuffers[i], m_shaderStorageBuffersMemory[i]);
            m_device->copyBuffer(stagingBuffer, m_shaderStorageBuffers[i], bufferSize);
        }

        vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);
    }

    void createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBuffersMemory[i]);
            vkMapMemory(m_device->getDevice(), m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_uniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(m_device->getDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool.");
    }

    void createComputeDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_computeDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        m_computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(m_device->getDevice(), &allocInfo, m_computeDescriptorSets.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate descriptor sets.");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_computeDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0; // uniform buffer binding index
            descriptorWrites[0].dstArrayElement = 0; // first item in array (ubo) to update
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            VkDescriptorBufferInfo lastFrameInfo{};
            lastFrameInfo.buffer = m_shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
            lastFrameInfo.offset = 0;
            lastFrameInfo.range = sizeof(Particle) * m_particleCount;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_computeDescriptorSets[i];
            descriptorWrites[1].dstBinding = 1; // uniform buffer binding index
            descriptorWrites[1].dstArrayElement = 0; // first item in array (ubo) to update
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &lastFrameInfo;

            VkDescriptorBufferInfo currentFrameInfo{};
            currentFrameInfo.buffer = m_shaderStorageBuffers[i];
            currentFrameInfo.offset = 0;
            currentFrameInfo.range = sizeof(Particle) * m_particleCount;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = m_computeDescriptorSets[i];
            descriptorWrites[2].dstBinding = 2; // uniform buffer binding index
            descriptorWrites[2].dstArrayElement = 0; // first item in array (ubo) to update
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo = &currentFrameInfo;

            vkUpdateDescriptorSets(m_device->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void createRenderCommandBuffers()
    {
        m_renderCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_device->getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_renderCommandBuffers.size());

        if (vkAllocateCommandBuffers(m_device->getDevice(), &allocInfo, m_renderCommandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command buffers.");
    }

    void createComputeCommandBuffers()
    {
        m_computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_device->getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_computeCommandBuffers.size());

        if (vkAllocateCommandBuffers(m_device->getDevice(), &allocInfo, m_computeCommandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate compute command buffers.");
    }

    void createSyncObjects()
    {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_device->getDevice(), &fenceInfo, nullptr, &m_renderInFlightFences[i]) != VK_SUCCESS)
                    throw std::runtime_error("Failed to create render synchronisation objects for a frame.");
            if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_computeFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_device->getDevice(), &fenceInfo, nullptr, &m_computeInFlightFences[i]) != VK_SUCCESS)
                    throw std::runtime_error("Failed to create compute synchronisation objects for a frame.");
        }
    }

    void drawFrame()
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        vkWaitForFences(m_device->getDevice(), 1, &m_computeInFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        updateUniformBuffer(m_currentFrame);

        vkResetFences(m_device->getDevice(), 1, &m_computeInFlightFences[m_currentFrame]);

        vkResetCommandBuffer(m_computeCommandBuffers[m_currentFrame], 0);
        recordComputeCommandBuffer(m_computeCommandBuffers[m_currentFrame]);

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_computeCommandBuffers[m_currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_computeFinishedSemaphores[m_currentFrame];
        
        if (vkQueueSubmit(m_device->getComputeQueue(), 1, &submitInfo, m_computeInFlightFences[m_currentFrame]) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit compute command buffer.");

        vkWaitForFences(m_device->getDevice(), 1, &m_renderInFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_device->getDevice(), m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swap chain image.");

        vkResetFences(m_device->getDevice(), 1, &m_renderInFlightFences[m_currentFrame]);

        vkResetCommandBuffer(m_renderCommandBuffers[m_currentFrame], 0);
        recordRenderCommandBuffer(m_renderCommandBuffers[m_currentFrame], imageIndex);

        VkSemaphore waitSemaphores[] = { m_computeFinishedSemaphores[m_currentFrame], m_imageAvailableSemaphores[m_currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 2;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_renderCommandBuffers[m_currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_device->getRenderQueue(), 1, &submitInfo, m_renderInFlightFences[m_currentFrame]) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command buffer.");

        VkSwapchainKHR swapChains[] = { m_swapChain };
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // optional

        result = vkQueuePresentKHR(m_device->getPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
        {
            m_framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to present swap chain image.");

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return availableFormat;
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return availablePresentMode;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;

        VkExtent2D actualExtent = m_device->getWindow()->getExtent();

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(m_device->getDevice(), &ci, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module.");

        return shaderModule;
    }

    void recordRenderCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // optional
        beginInfo.pInheritanceInfo = nullptr; // optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer.");

        VkClearValue clearColour = {{{ 0.f, 0.f, 0.f, 1.f }}};
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapChainExtent;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColour;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // vkCmd commands return void so no error handling until recording is finished
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_renderPipeline);

        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(m_swapChainExtent.width);
        viewport.height = static_cast<float>(m_swapChainExtent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapChainExtent;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_shaderStorageBuffers[m_currentFrame], offsets);
        vkCmdDraw(commandBuffer, m_particleCount, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to record render command buffer.");
    }

    void recordComputeCommandBuffer(VkCommandBuffer commandBuffer)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("failed to begin recording compute command buffer!");

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_computeDescriptorSets[m_currentFrame], 0, nullptr);
        vkCmdDispatch(commandBuffer, m_particleCount / 256, 1, 1);
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record compute command buffer.");
        }
    }

    void updateUniformBuffer(uint32_t currentImage)
    {
        UniformBufferObject ubo{};
        ubo.u_deltaTime = m_lastFrameTime * 2.f;
        memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    static std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
            throw std::runtime_error("Failed to open file.");

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->m_framebufferResized = true;
    }

private:
    Device* m_device;
    const uint32_t m_particleCount = 8192;

    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    
    VkRenderPass m_renderPass;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_renderDescriptorSets;
    std::vector<VkDescriptorSet> m_computeDescriptorSets;
    VkDescriptorSetLayout m_renderDescriptorSetLayout;
    VkPipelineLayout m_renderPipelineLayout;
    VkPipeline m_renderPipeline;
    VkDescriptorSetLayout m_computeDescriptorSetLayout;
    VkPipelineLayout m_computePipelineLayout;
    VkPipeline m_computePipeline;
    std::vector<VkCommandBuffer> m_renderCommandBuffers;
    std::vector<VkCommandBuffer> m_computeCommandBuffers;
    
    std::vector<VkBuffer> m_shaderStorageBuffers;
    std::vector<VkDeviceMemory> m_shaderStorageBuffersMemory;

    uint32_t m_mipLevels;
    VkImage m_textureImage;
    VkDeviceMemory m_textureImageMemory;
    VkImageView m_textureImageView;
    VkSampler m_textureSampler;
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;
    
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkSemaphore> m_computeFinishedSemaphores;
    std::vector<VkFence> m_renderInFlightFences;
    std::vector<VkFence> m_computeInFlightFences;
    
    uint32_t m_currentFrame = 0;
    float m_lastFrameTime = 0.f;
    double m_lastTime = 0.f;
    bool m_framebufferResized = false;
};

int main() {
    Application app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
