/** \file computeApp.cpp */

#include "examples/computeApp.hpp"

void ComputeApp::initApplication()
{
    m_device = new Device();
    m_descriptorManager = new DescriptorManager(m_device, Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_renderer = new Renderer(m_device);
    m_lastTime = glfwGetTime();

    createComputeDescriptorSetLayout();
    createGraphicsPipeline();
    createComputePipeline();
    createShaderStorageBuffers();
    createUniformBuffers();
    createDescriptorPool();
    createComputeDescriptorSets();
}

void ComputeApp::mainLoop()
{
    while (!m_device->getWindow()->shouldClose())
    {
        glfwPollEvents();
        drawFrame();
        double currentTime = glfwGetTime();
        m_lastFrameTime = (currentTime - m_lastTime) * 1000.f;
        m_lastTime = currentTime;

        if (m_device->isKeyPressed(GLFW_KEY_ESCAPE))
        {
            std::cout << "[EventSystem] Exiting..." << std::endl;
            m_device->closeWindow();
        }
    }

    vkDeviceWaitIdle(m_device->getDevice());
}

void ComputeApp::cleanup()
{
    m_graphicsPipeline->destroyPipelineLayout();
    m_computePipeline->destroyPipelineLayout();
    for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(m_device->getDevice(), m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_device->getDevice(), m_uniformBuffersMemory[i], nullptr);
    }
    for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(m_device->getDevice(), m_shaderStorageBuffers[i], nullptr);
        vkFreeMemory(m_device->getDevice(), m_shaderStorageBuffersMemory[i], nullptr);
    }
    delete m_descriptorManager;
    m_descriptorManager = nullptr;
    delete m_graphicsPipeline;
    m_graphicsPipeline = nullptr;
    delete m_computePipeline;
    m_computePipeline = nullptr;
    delete m_renderer;
    m_renderer = nullptr;
    delete m_device;
    m_device = nullptr;
}

void ComputeApp::drawFrame()
{
    vkWaitForFences(m_device->getDevice(), 1, &m_renderer->getComputeInFlightFence(), VK_TRUE, UINT64_MAX);
    updateUniformBuffer(m_renderer->getCurrentFrame());
    vkResetFences(m_device->getDevice(), 1, &m_renderer->getComputeInFlightFence());
    vkResetCommandBuffer(m_renderer->getComputeCommandBuffer(), 0);
    m_renderer->recordCommandBuffer(true, m_computePipeline, m_particleCount, {}, m_descriptorManager->getDescriptorSets(true));
    m_renderer->submitCommandBuffer(true);

    vkWaitForFences(m_device->getDevice(), 1, &m_renderer->getGraphicsInFlightFence(), VK_TRUE, UINT64_MAX);
    m_renderer->beginFrame();
    vkResetFences(m_device->getDevice(), 1, &m_renderer->getGraphicsInFlightFence());
    vkResetCommandBuffer(m_renderer->getGraphicsCommandBuffer(), 0);
    m_renderer->recordCommandBuffer(false, m_graphicsPipeline, m_particleCount, m_shaderStorageBuffers, {});
    m_renderer->submitCommandBuffer(false);

    m_renderer->endFrame();
}

void ComputeApp::createComputeDescriptorSetLayout()
{
    m_descriptorManager->addBinding(true, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    m_descriptorManager->addBinding(true, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    m_descriptorManager->addBinding(true, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    m_descriptorManager->buildDescriptorSetLayout(true);
}

void ComputeApp::createGraphicsPipeline()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // optional

    PipelineSettings pipelineSettings{};
    Pipeline::defaultPipelineSettings(pipelineSettings);
    pipelineSettings.bindingDescription = Particle::getBindingDescription();
    pipelineSettings.attributeDescriptions = Particle::getAttributeDescriptions();
    pipelineSettings.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    pipelineSettings.rasteriser.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineSettings.rasteriser.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // counter clockwise due to the Y-flip in the projection matrix
    Pipeline::enableAlphaBlending(pipelineSettings);
    pipelineSettings.renderPass = m_renderer->getSwapchainRenderPass();
    pipelineSettings.subpass = 0;

    m_graphicsPipeline = new Pipeline(m_device, pipelineLayoutInfo, pipelineSettings, "./res/shaders/computeApp/vert.spv", "./res/shaders/computeApp/frag.spv");
}

void ComputeApp::createComputePipeline()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = m_descriptorManager->getComputeDescriptorSetLayout();
    pipelineLayoutInfo.pushConstantRangeCount = 0; // optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // optional

    PipelineSettings pipelineSettings{};

    m_computePipeline = new Pipeline(m_device, pipelineLayoutInfo, pipelineSettings, "./res/shaders/computeApp/comp.spv");
}

void ComputeApp::createShaderStorageBuffers()
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

    m_shaderStorageBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_shaderStorageBuffersMemory.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

    // copy initial particle data to storage buffer
    for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_device->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_shaderStorageBuffers[i], m_shaderStorageBuffersMemory[i]);
        m_device->copyBuffer(stagingBuffer, m_shaderStorageBuffers[i], bufferSize);
    }

    vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);
}

void ComputeApp::createUniformBuffers()
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

void ComputeApp::createDescriptorPool()
{
    m_descriptorManager->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT));
    m_descriptorManager->addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT) * 2);
    m_descriptorManager->buildDescriptorPool();
}

void ComputeApp::createComputeDescriptorSets()
{
    m_descriptorManager->allocateDescriptorSets(true);

    for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        m_descriptorManager->addWriteDescriptorSet(true, 0, &bufferInfo, nullptr);

        VkDescriptorBufferInfo lastFrameInfo{};
        lastFrameInfo.buffer = m_shaderStorageBuffers[(i - 1) % Swapchain::MAX_FRAMES_IN_FLIGHT];
        lastFrameInfo.offset = 0;
        lastFrameInfo.range = sizeof(Particle) * m_particleCount;

        m_descriptorManager->addWriteDescriptorSet(true, 1, &lastFrameInfo, nullptr);

        VkDescriptorBufferInfo currentFrameInfo{};
        currentFrameInfo.buffer = m_shaderStorageBuffers[i];
        currentFrameInfo.offset = 0;
        currentFrameInfo.range = sizeof(Particle) * m_particleCount;

        m_descriptorManager->addWriteDescriptorSet(true, 2, &currentFrameInfo, nullptr);

        m_descriptorManager->overwrite(true, i);
    }
}

void ComputeApp::updateUniformBuffer(uint32_t currentImage)
{
    UniformBufferObject ubo{};
    ubo.u_deltaTime = m_lastFrameTime * 2.f;
    memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
