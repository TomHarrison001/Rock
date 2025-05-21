/** \file modelApp.cpp */

#include "examples/modelApp.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

void ModelApp::initApplication()
{
    m_device = new Device();
    m_msaaSamples = m_device->getMaxUsableSampleCount();
    m_descriptorManager = new DescriptorManager(m_device, Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_renderer = new Renderer(m_device, m_msaaSamples, true);

    createDescriptorSetLayout();
    createGraphicsPipeline();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
}

void ModelApp::mainLoop()
{
    while (!m_device->getWindow()->shouldClose())
    {
        glfwPollEvents();
        drawFrame();

        if (m_device->isKeyPressed(GLFW_KEY_ESCAPE))
        {
            std::cout << "[EventSystem] Exiting..." << std::endl;
            m_device->closeWindow();
        }
    }

    vkDeviceWaitIdle(m_device->getDevice());
}

void ModelApp::cleanup()
{
    m_graphicsPipeline->destroyPipelineLayout();
    vkDestroySampler(m_device->getDevice(), m_textureSampler, nullptr);
    vkDestroyImageView(m_device->getDevice(), m_textureImageView, nullptr);
    vkDestroyImage(m_device->getDevice(), m_textureImage, nullptr);
    vkFreeMemory(m_device->getDevice(), m_textureImageMemory, nullptr);
    vkDestroyBuffer(m_device->getDevice(), m_vertexBuffer, nullptr);
    vkFreeMemory(m_device->getDevice(), m_vertexBufferMemory, nullptr);
    vkDestroyBuffer(m_device->getDevice(), m_indexBuffer, nullptr);
    vkFreeMemory(m_device->getDevice(), m_indexBufferMemory, nullptr);
    for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(m_device->getDevice(), m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_device->getDevice(), m_uniformBuffersMemory[i], nullptr);
    }
    delete m_descriptorManager;
    m_descriptorManager = nullptr;
    delete m_graphicsPipeline;
    m_graphicsPipeline = nullptr;
    delete m_renderer;
    m_renderer = nullptr;
    delete m_device;
    m_device = nullptr;
}

void ModelApp::drawFrame()
{
    vkWaitForFences(m_device->getDevice(), 1, &m_renderer->getInFlightFence(), VK_TRUE, UINT64_MAX);
    m_renderer->beginFrame();
    updateUniformBuffer(m_renderer->getCurrentFrame());
    vkResetFences(m_device->getDevice(), 1, &m_renderer->getInFlightFence());
    vkResetCommandBuffer(m_renderer->getGraphicsCommandBuffer(), 0);
    m_renderer->recordCommandBuffer(m_graphicsPipeline, m_vertexBuffer, m_indexBuffer, m_descriptorManager->getDescriptorSets(), m_indices);
    m_renderer->submitCommandBuffer();

    m_renderer->endFrame();
}

void ModelApp::createDescriptorSetLayout()
{
    m_descriptorManager->addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    m_descriptorManager->addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_descriptorManager->buildDescriptorSetLayout();
}

void ModelApp::createGraphicsPipeline()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = m_descriptorManager->getDescriptorSetLayout();
    pipelineLayoutInfo.pushConstantRangeCount = 0; // optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // optional

    PipelineSettings pipelineSettings{};
    Pipeline::defaultPipelineSettings(pipelineSettings);
    pipelineSettings.bindingDescription = Vertex::getBindingDescription();
    pipelineSettings.attributeDescriptions = Vertex::getAttributeDescriptions();
    pipelineSettings.rasteriser.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineSettings.rasteriser.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // counter clockwise due to the Y-flip in the projection matrix
    pipelineSettings.multisampling.rasterizationSamples = m_msaaSamples;
    pipelineSettings.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineSettings.depthStencil.depthTestEnable = VK_TRUE;
    pipelineSettings.depthStencil.depthWriteEnable = VK_TRUE;
    pipelineSettings.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineSettings.depthStencil.depthBoundsTestEnable = VK_FALSE;
    pipelineSettings.depthStencil.minDepthBounds = 0.f; // optional
    pipelineSettings.depthStencil.maxDepthBounds = 1.f; // optional
    pipelineSettings.depthStencil.stencilTestEnable = VK_FALSE;
    pipelineSettings.depthStencil.front = {}; // optional
    pipelineSettings.depthStencil.back = {}; // optional
    pipelineSettings.renderPass = m_renderer->getSwapchainRenderPass();
    pipelineSettings.subpass = 0;

    m_graphicsPipeline = new Pipeline(m_device, pipelineLayoutInfo, pipelineSettings, "./res/shaders/modelApp/vert.spv", "./res/shaders/modelApp/frag.spv");
}

void ModelApp::createTextureImage()
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("./res/textures/ironGolem.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels)
        throw std::runtime_error("Failed to load texture image.");

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    m_device->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device->getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device->getDevice(), stagingBufferMemory);

    stbi_image_free(pixels);

    m_device->createImage(texWidth, texHeight, m_mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory);

    transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
    copyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);

    generateMipmaps(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_mipLevels);
}

void ModelApp::createTextureImageView()
{
    m_textureImageView = m_device->createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);
}

void ModelApp::createTextureSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_device->getPhysicalDevice(), &properties);

    VkSamplerCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci.magFilter = VK_FILTER_LINEAR;
    ci.minFilter = VK_FILTER_LINEAR;
    ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.anisotropyEnable = VK_TRUE;
    ci.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    ci.unnormalizedCoordinates = VK_FALSE;
    ci.compareEnable = VK_FALSE;
    ci.compareOp = VK_COMPARE_OP_ALWAYS;
    ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ci.mipLodBias = 0.f;
    ci.minLod = 0.f;
    ci.maxLod = VK_LOD_CLAMP_NONE;

    if (vkCreateSampler(m_device->getDevice(), &ci, nullptr, &m_textureSampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler.");
}

void ModelApp::loadModel()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "./res/models/ironGolem.obj"))
        throw std::runtime_error(warn + err);

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0] * 0.04f, // scale: 0.04f
                attrib.vertices[3 * index.vertex_index + 1] * 0.04f, // scale: 0.04f
                attrib.vertices[3 * index.vertex_index + 2] * 0.04f  // scale: 0.04f
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index],
                1.f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.colour = { 1.f, 1.f, 1.f };

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
            }

            m_indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void ModelApp::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(Vertex) * m_vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_device->getDevice(), stagingBufferMemory);

    m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

    m_device->copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);
}

void ModelApp::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(uint32_t) * m_indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_indices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_device->getDevice(), stagingBufferMemory);

    m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

    m_device->copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

    vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);
}

void ModelApp::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMemory.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMapped.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBuffersMemory[i]);
        vkMapMemory(m_device->getDevice(), m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_uniformBuffersMapped[i]);
    }
}

void ModelApp::createDescriptorPool()
{
    m_descriptorManager->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT));
    m_descriptorManager->addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT));
    m_descriptorManager->buildDescriptorPool();
}

void ModelApp::createDescriptorSets()
{
    m_descriptorManager->allocateDescriptorSets();

    for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        m_descriptorManager->addWriteDescriptorSet(0, &bufferInfo, nullptr);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;

        m_descriptorManager->addWriteDescriptorSet(1, nullptr, &imageInfo);

        m_descriptorManager->overwrite(i);
    }
}

VkCommandBuffer ModelApp::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_device->getCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device->getDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void ModelApp::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_device->getGraphicsQueue());
    vkFreeCommandBuffers(m_device->getDevice(), m_device->getCommandPool(), 1, &commandBuffer);
}

void ModelApp::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void ModelApp::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    //ubo.model = glm::rotate(glm::mat4(1.f), time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
    ubo.model = glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f)) * glm::rotate(glm::mat4(1.f), time * glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
    ubo.view = glm::lookAt(glm::vec3(2.f), glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
    ubo.proj = glm::perspective(glm::radians(45.f), m_renderer->getSwapchainAspectRatio(), 0.1f, 10.f);
    ubo.proj[1][1] *= -1.f; // image would be renderered upside down otherwise due to glm being originally designed for OpenGL where the Y coord is inverted

    memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void ModelApp::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (m_device->hasStencilComponent(format))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
        throw std::runtime_error("Unsupported layout transition.");

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

void ModelApp::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(m_device->getPhysicalDevice(), imageFormat, &properties);

    if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        throw std::runtime_error("Texture image format does not support linear blitting.");

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < m_mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}
