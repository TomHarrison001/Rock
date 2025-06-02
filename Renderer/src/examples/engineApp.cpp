/** \file engineApp.cpp */

#include "examples/engineApp.hpp"

//#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

//#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

void EngineApp::initApplication()
{
    m_device = new Device();
    m_msaaSamples = m_device->getMaxUsableSampleCount();
    m_descriptorManager = new DescriptorManager(m_device, Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_renderer = new Renderer(m_device, m_msaaSamples, true);

    createDescriptorSetLayouts();
    createGraphicsPipeline();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    m_gameObject = m_registry.create();
    m_registry.emplace<Rock::RenderComponent>(m_gameObject);
    m_registry.emplace<Rock::TransformComponent>(m_gameObject, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f), glm::vec3(1.f));
    m_registry.emplace<Rock::OBBComponent>(m_gameObject, glm::vec3(0.5f));
    loadTexture(m_gameObject, "./res/textures/player.png");
    loadModel(m_gameObject, "./res/models/player.obj");
    m_floor = m_registry.create();
    m_registry.emplace<Rock::RenderComponent>(m_floor);
    m_registry.emplace<Rock::TransformComponent>(m_floor, glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f), glm::vec3(3.f, 0.1f, 3.f));
    m_registry.emplace<Rock::OBBComponent>(m_floor, glm::vec3(0.5f));
    loadTexture(m_floor, "./res/textures/cube2.png");
    loadModel(m_floor, "./res/models/cube.obj");

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForVulkan(m_device->getWindow()->getWindow(), true);
    ImGui_ImplVulkan_InitInfo info{};
    info.DescriptorPool = m_descriptorManager->getDescriptorPool();
    info.RenderPass = m_renderer->getSwapchainRenderPass();
    info.Device = m_device->getDevice();
    info.PhysicalDevice = m_device->getPhysicalDevice();
    info.MinImageCount = Swapchain::MAX_FRAMES_IN_FLIGHT;
    info.ImageCount = Swapchain::MAX_FRAMES_IN_FLIGHT;
    info.MSAASamples = m_msaaSamples;
    info.Instance = m_device->getInstance();
    info.Queue = m_device->getGraphicsQueue();
    ImGui_ImplVulkan_Init(&info);

    // load font
    io.Fonts->AddFontFromFileTTF("./res/fonts/Segoe-UI-Variable-Static-Text-Semibold.ttf", 16.f);

    vkDeviceWaitIdle(m_device->getDevice());
}

void EngineApp::mainLoop()
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

        auto& transformComp = m_registry.get<Rock::TransformComponent>(m_gameObject);
        transformComp.m_translation = glm::vec3(m_translate[0], m_translate[1], m_translate[2]);
        transformComp.m_rotation = glm::vec3(glm::radians(m_rotate[0]), glm::radians(m_rotate[1]), glm::radians(m_rotate[2]));
        transformComp.m_scale = glm::vec3(m_scale[0], m_scale[1], m_scale[2]);
        transformComp.recalculate();
    }

    vkDeviceWaitIdle(m_device->getDevice());
}

void EngineApp::cleanup()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_graphicsPipeline->destroyPipelineLayout();
    std::vector<entt::entity> m_gameObjects = { m_gameObject, m_floor };
    for (entt::entity entity : m_gameObjects)
    {
        auto& renderComp = m_registry.get<Rock::RenderComponent>(entity);
        vkDestroySampler(m_device->getDevice(), renderComp.m_textureSampler, nullptr);
        vkDestroyImageView(m_device->getDevice(), renderComp.m_textureImageView, nullptr);
        vkDestroyImage(m_device->getDevice(), renderComp.m_textureImage, nullptr);
        vkFreeMemory(m_device->getDevice(), renderComp.m_textureImageMemory, nullptr);
        vkDestroyBuffer(m_device->getDevice(), renderComp.m_vertexBuffer, nullptr);
        vkDestroyBuffer(m_device->getDevice(), renderComp.m_indexBuffer, nullptr);
        vkFreeMemory(m_device->getDevice(), renderComp.m_vertexBufferMemory, nullptr);
        vkFreeMemory(m_device->getDevice(), renderComp.m_indexBufferMemory, nullptr);
    }
    for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(m_device->getDevice(), m_cameraBuffers[i], nullptr);
        vkDestroyBuffer(m_device->getDevice(), m_lightBuffers[i], nullptr);
        vkDestroyBuffer(m_device->getDevice(), m_viewPosBuffers[i], nullptr);
        vkFreeMemory(m_device->getDevice(), m_cameraBuffersMemory[i], nullptr);
        vkFreeMemory(m_device->getDevice(), m_lightBuffersMemory[i], nullptr);
        vkFreeMemory(m_device->getDevice(), m_viewPosBuffersMemory[i], nullptr);
    }
    vkDestroyDescriptorSetLayout(m_device->getDevice(), m_textureDescriptorSetLayout, nullptr);
    delete m_descriptorManager;
    m_descriptorManager = nullptr;
    delete m_graphicsPipeline;
    m_graphicsPipeline = nullptr;
    delete m_renderer;
    m_renderer = nullptr;
    delete m_device;
    m_device = nullptr;
}

void EngineApp::drawFrame()
{
    vkWaitForFences(m_device->getDevice(), 1, &m_renderer->getFence(), VK_TRUE, UINT64_MAX);
    m_renderer->beginFrame();
    updateUniformBuffer(m_renderer->getCurrentFrame());
    vkResetFences(m_device->getDevice(), 1, &m_renderer->getFence());
    vkResetCommandBuffer(m_renderer->getCommandBuffer(), 0);
    std::vector<entt::entity> m_gameObjects = { m_gameObject, m_floor };
    m_renderer->recordCommandBuffer(m_graphicsPipeline, m_registry, m_gameObjects, m_descriptorManager->getDescriptorSets(), m_translate, m_rotate, m_scale);
    m_renderer->submitCommandBuffer();

    m_renderer->endFrame();
}

void EngineApp::createDescriptorSetLayouts()
{
    m_descriptorManager->addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    m_descriptorManager->addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_descriptorManager->addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_descriptorManager->buildDescriptorSetLayout();

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(m_device->getDevice(), &layoutInfo, nullptr, &m_textureDescriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture descriptor set layout.");
}

void EngineApp::createGraphicsPipeline()
{
    VkPushConstantRange psRange;
    psRange.offset = 0;
    psRange.size = sizeof(glm::mat4);
    psRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayout setLayouts[] = { *m_descriptorManager->getDescriptorSetLayout(), m_textureDescriptorSetLayout };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &psRange;

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

    m_graphicsPipeline = new Pipeline(m_device, pipelineLayoutInfo, pipelineSettings, "./res/shaders/engineApp/vert.spv", "./res/shaders/engineApp/frag.spv");
}

void EngineApp::loadTexture(entt::entity entity, const char* path)
{
    auto& renderComp = m_registry.get<Rock::RenderComponent>(entity);
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    renderComp.m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

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

    m_device->createImage(texWidth, texHeight, renderComp.m_mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderComp.m_textureImage, renderComp.m_textureImageMemory);

    transitionImageLayout(renderComp.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, renderComp.m_mipLevels);
    copyBufferToImage(stagingBuffer, renderComp.m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);

    generateMipmaps(renderComp.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, renderComp.m_mipLevels);

    createTextureImageView(entity);
    createTextureSampler(entity);
    createTextureDescriptorSet(entity);
}

void EngineApp::createTextureImageView(entt::entity entity)
{
    auto& renderComp = m_registry.get<Rock::RenderComponent>(entity);
    renderComp.m_textureImageView = m_device->createImageView(renderComp.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, renderComp.m_mipLevels);
}

void EngineApp::createTextureSampler(entt::entity entity)
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

    auto& renderComp = m_registry.get<Rock::RenderComponent>(entity);
    if (vkCreateSampler(m_device->getDevice(), &ci, nullptr, &renderComp.m_textureSampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler.");
}

void EngineApp::loadModel(entt::entity entity, const char* path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path))
        throw std::runtime_error(warn + err);

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.norm = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index],
                1.f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    auto& renderComp = m_registry.get<Rock::RenderComponent>(entity);
    renderComp.m_vertices = vertices;
    renderComp.m_indices = indices;

    createVertexBuffer(entity);
    createIndexBuffer(entity);
}

void EngineApp::createVertexBuffer(entt::entity entity)
{
    auto& renderComp = m_registry.get<Rock::RenderComponent>(entity);
    VkDeviceSize bufferSize = sizeof(Vertex) * renderComp.m_vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, renderComp.m_vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_device->getDevice(), stagingBufferMemory);

    m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderComp.m_vertexBuffer, renderComp.m_vertexBufferMemory);

    m_device->copyBuffer(stagingBuffer, renderComp.m_vertexBuffer, bufferSize);

    vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);
}

void EngineApp::createIndexBuffer(entt::entity entity)
{
    auto& renderComp = m_registry.get<Rock::RenderComponent>(entity);
    VkDeviceSize bufferSize = sizeof(uint32_t) * renderComp.m_indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, renderComp.m_indices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_device->getDevice(), stagingBufferMemory);

    m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderComp.m_indexBuffer, renderComp.m_indexBufferMemory);

    m_device->copyBuffer(stagingBuffer, renderComp.m_indexBuffer, bufferSize);

    vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);
}

void EngineApp::createUniformBuffers()
{
    m_cameraBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_lightBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_viewPosBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_cameraBuffersMemory.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_lightBuffersMemory.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_viewPosBuffersMemory.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_cameraBuffersMapped.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_lightBuffersMapped.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_viewPosBuffersMapped.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_device->createBuffer(sizeof(CameraUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_cameraBuffers[i], m_cameraBuffersMemory[i]);
        vkMapMemory(m_device->getDevice(), m_cameraBuffersMemory[i], 0, sizeof(CameraUBO), 0, &m_cameraBuffersMapped[i]);
        m_device->createBuffer(sizeof(LightUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_lightBuffers[i], m_lightBuffersMemory[i]);
        vkMapMemory(m_device->getDevice(), m_lightBuffersMemory[i], 0, sizeof(LightUBO), 0, &m_lightBuffersMapped[i]);
        m_device->createBuffer(sizeof(ViewUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_viewPosBuffers[i], m_viewPosBuffersMemory[i]);
        vkMapMemory(m_device->getDevice(), m_viewPosBuffersMemory[i], 0, sizeof(ViewUBO), 0, &m_viewPosBuffersMapped[i]);
    }
}

void EngineApp::createDescriptorPool()
{
    m_descriptorManager->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT));
    m_descriptorManager->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT));
    m_descriptorManager->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT));
    m_descriptorManager->addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT * 2 + 1));
    m_descriptorManager->buildDescriptorPool();
}

void EngineApp::createDescriptorSets()
{
    m_descriptorManager->allocateDescriptorSets();

    for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo cameraBufferInfo{};
        cameraBufferInfo.buffer = m_cameraBuffers[i];
        cameraBufferInfo.offset = 0;
        cameraBufferInfo.range = sizeof(CameraUBO);

        m_descriptorManager->addWriteDescriptorSet(0, &cameraBufferInfo, nullptr);

        VkDescriptorBufferInfo lightBufferInfo{};
        lightBufferInfo.buffer = m_lightBuffers[i];
        lightBufferInfo.offset = 0;
        lightBufferInfo.range = sizeof(LightUBO);

        m_descriptorManager->addWriteDescriptorSet(1, &lightBufferInfo, nullptr);

        VkDescriptorBufferInfo viewPosBufferInfo{};
        viewPosBufferInfo.buffer = m_viewPosBuffers[i];
        viewPosBufferInfo.offset = 0;
        viewPosBufferInfo.range = sizeof(ViewUBO);

        m_descriptorManager->addWriteDescriptorSet(2, &viewPosBufferInfo, nullptr);

        m_descriptorManager->overwrite(i);
    }
}

void EngineApp::createTextureDescriptorSet(entt::entity entity)
{
    auto& renderComp = m_registry.get<Rock::RenderComponent>(entity);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorManager->getDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_textureDescriptorSetLayout;

    if (vkAllocateDescriptorSets(m_device->getDevice(), &allocInfo, &renderComp.descriptorSet) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor set.");

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = renderComp.m_textureImageView;
    imageInfo.sampler = renderComp.m_textureSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = renderComp.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device->getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void EngineApp::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    CameraUBO u_camera{};
    u_camera.view = glm::lookAt(glm::vec3(2.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    u_camera.proj = glm::perspective(glm::radians(45.f), m_renderer->getSwapchainAspectRatio(), 0.1f, 10.f);
    u_camera.proj[1][1] *= -1.f; // image would be renderered upside down otherwise due to glm being originally designed for OpenGL where the Y coord is inverted
    memcpy(m_cameraBuffersMapped[currentImage], &u_camera, sizeof(u_camera));

    LightUBO u_light{};
    u_light.dLight.colour = glm::vec3(1.f, 1.f, 0.f);
    u_light.dLight.direction = glm::vec3(-1.f, -1.f, -1.f);
    u_light.pLights[0].colour = glm::vec3(0.f, 0.f, 0.f);
    u_light.pLights[0].position = glm::vec3(1.f, 1.f, 1.f);
    u_light.pLights[0].constants = glm::vec3(1.f, 0.1f, 0.01f);
    memcpy(m_lightBuffersMapped[currentImage], &u_light, sizeof(u_light));

    ViewUBO u_viewPos{};
    u_viewPos.viewPos = glm::vec3(2.f, 2.f, 2.f);
    memcpy(m_viewPosBuffersMapped[currentImage], &u_viewPos, sizeof(u_viewPos));
}

void EngineApp::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
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

    for (uint32_t i = 1; i < mipLevels; i++)
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

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}
