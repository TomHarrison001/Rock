#include <limits>
#include <array>
#include <random>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rendering/swapchain.hpp"
#include "rendering/pipeline.hpp"

struct UniformBufferObject {
    float u_deltaTime = 1.f;
};

class Application
{
public:
    void run()
    {
        initApplication();
        mainLoop();
        cleanup();
    }

private:
    void initApplication()
    {
        m_device = new Device();
        m_swapchain = new Swapchain(m_device);
        m_lastTime = glfwGetTime();

        createComputeDescriptorSetLayout();
        createRenderPipeline();
        createComputePipeline();
        createShaderStorageBuffers();
        createUniformBuffers();
        createDescriptorPool();
        createComputeDescriptorSets();
        createRenderCommandBuffers();
        createComputeCommandBuffers();
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
        vkDestroyPipelineLayout(m_device->getDevice(), m_renderPipelineLayout, nullptr);
        vkDestroyPipelineLayout(m_device->getDevice(), m_computePipelineLayout, nullptr);
        for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(m_device->getDevice(), m_uniformBuffers[i], nullptr);
            vkFreeMemory(m_device->getDevice(), m_uniformBuffersMemory[i], nullptr);
        }
        vkDestroyDescriptorPool(m_device->getDevice(), m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device->getDevice(), m_computeDescriptorSetLayout, nullptr);
        for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(m_device->getDevice(), m_shaderStorageBuffers[i], nullptr);
            vkFreeMemory(m_device->getDevice(), m_shaderStorageBuffersMemory[i], nullptr);
        }
        delete m_graphicsPipeline;
        m_graphicsPipeline = nullptr;
        delete m_computePipeline;
        m_computePipeline = nullptr;
        //delete m_device;
        m_device = nullptr;
        m_swapchain = nullptr;
    }

    void recreateSwapChain()
    {
        VkExtent2D extent = m_device->getWindow()->getExtent();
        while (extent.width == 0 || extent.height == 0)
        {
            extent = m_device->getWindow()->getExtent();
            glfwWaitEvents();
    }

        vkDeviceWaitIdle(m_device->getDevice());

        if (m_swapchain != nullptr)
        {
            Swapchain* oldSwapchain = std::move(m_swapchain);
            m_swapchain = new Swapchain(m_device, oldSwapchain);
            if (*oldSwapchain != m_swapchain)
                throw std::runtime_error("Swap chain image/depth format has changed.");
            delete oldSwapchain;
            oldSwapchain = nullptr;
        }
        else
            m_swapchain = new Swapchain(m_device);
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
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // optional

        if (vkCreatePipelineLayout(m_device->getDevice(), &pipelineLayoutInfo, nullptr, &m_renderPipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pipeline layout.");

        PipelineSettings pipelineSettings{};
        Pipeline::defaultPipelineSettings(pipelineSettings);
        pipelineSettings.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        pipelineSettings.rasteriser.cullMode = VK_CULL_MODE_BACK_BIT;
        pipelineSettings.rasteriser.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // counter clockwise due to the Y-flip in the projection matrix
        pipelineSettings.colourBlendAttachment.blendEnable = VK_TRUE;
        pipelineSettings.colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        pipelineSettings.colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipelineSettings.colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipelineSettings.colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineSettings.pipelineLayout = m_renderPipelineLayout;
        pipelineSettings.renderPass = m_swapchain->getRenderPass();
        pipelineSettings.subpass = 0;

        m_graphicsPipeline = new Pipeline(m_device, pipelineSettings, "./res/shaders/vert.spv", "./res/shaders/frag.spv");
    }

    void createComputePipeline()
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_computeDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // optional

        if (vkCreatePipelineLayout(m_device->getDevice(), &pipelineLayoutInfo, nullptr, &m_computePipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute pipeline layout.");

        PipelineSettings pipelineSettings{};
        pipelineSettings.pipelineLayout = m_computePipelineLayout;

        m_computePipeline = new Pipeline(m_device, pipelineSettings, "./res/shaders/comp.spv");
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

    void createUniformBuffers()
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

    void createDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT) * 2;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(m_device->getDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool.");
    }

    void createComputeDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(Swapchain::MAX_FRAMES_IN_FLIGHT, m_computeDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        m_computeDescriptorSets.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(m_device->getDevice(), &allocInfo, m_computeDescriptorSets.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate descriptor sets.");

        for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
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
            lastFrameInfo.buffer = m_shaderStorageBuffers[(i - 1) % Swapchain::MAX_FRAMES_IN_FLIGHT];
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
        m_renderCommandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

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
        m_computeCommandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_device->getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_computeCommandBuffers.size());

        if (vkAllocateCommandBuffers(m_device->getDevice(), &allocInfo, m_computeCommandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate compute command buffers.");
    }

    void drawFrame()
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        vkWaitForFences(m_device->getDevice(), 1, &m_swapchain->getComputeInFlightFence(), VK_TRUE, UINT64_MAX);

        updateUniformBuffer(m_swapchain->getCurrentFrame());

        vkResetFences(m_device->getDevice(), 1, &m_swapchain->getComputeInFlightFence());

        vkResetCommandBuffer(m_computeCommandBuffers[m_swapchain->getCurrentFrame()], 0);
        recordComputeCommandBuffer(m_computeCommandBuffers[m_swapchain->getCurrentFrame()]);

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_computeCommandBuffers[m_swapchain->getCurrentFrame()];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_swapchain->getComputeFinishedSemaphore();

        if (vkQueueSubmit(m_device->getComputeQueue(), 1, &submitInfo, m_swapchain->getComputeInFlightFence()) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit compute command buffer.");

        vkWaitForFences(m_device->getDevice(), 1, &m_swapchain->getGraphicsInFlightFence(), VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_device->getDevice(), m_swapchain->getSwapchain(), UINT64_MAX, m_swapchain->getImageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swap chain image.");

        vkResetFences(m_device->getDevice(), 1, &m_swapchain->getGraphicsInFlightFence());

        vkResetCommandBuffer(m_renderCommandBuffers[m_swapchain->getCurrentFrame()], 0);
        recordRenderCommandBuffer(m_renderCommandBuffers[m_swapchain->getCurrentFrame()], imageIndex);

        VkSemaphore waitSemaphores[] = { m_swapchain->getComputeFinishedSemaphore(), m_swapchain->getImageAvailableSemaphore() };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore signalSemaphores[] = { m_swapchain->getGraphicsFinishedSemaphore() };
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 2;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_renderCommandBuffers[m_swapchain->getCurrentFrame()];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_device->getRenderQueue(), 1, &submitInfo, m_swapchain->getGraphicsInFlightFence()) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command buffer.");

        VkSwapchainKHR swapchains[] = { m_swapchain->getSwapchain()};
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
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

        m_swapchain->setCurrentFrame((m_swapchain->getCurrentFrame() + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT);
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
        renderPassInfo.renderPass = m_swapchain->getRenderPass();
        renderPassInfo.framebuffer = m_swapchain->getFramebuffer(imageIndex);
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapchain->getSwapchainExtent();
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColour;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // vkCmd commands return void so no error handling until recording is finished
        m_graphicsPipeline->bindGraphics(commandBuffer);

        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(m_swapchain->getSwapchainExtent().width);
        viewport.height = static_cast<float>(m_swapchain->getSwapchainExtent().height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapchain->getSwapchainExtent();

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_shaderStorageBuffers[m_swapchain->getCurrentFrame()], offsets);
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

        m_computePipeline->bindCompute(commandBuffer);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_computeDescriptorSets[m_swapchain->getCurrentFrame()], 0, nullptr);
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

private:
    Device* m_device;
    Swapchain* m_swapchain;
    Pipeline* m_graphicsPipeline;
    Pipeline* m_computePipeline;
    const uint32_t m_particleCount = 8192;
    
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_renderDescriptorSets;
    std::vector<VkDescriptorSet> m_computeDescriptorSets;
    VkDescriptorSetLayout m_renderDescriptorSetLayout;
    VkPipelineLayout m_renderPipelineLayout;
    VkDescriptorSetLayout m_computeDescriptorSetLayout;
    VkPipelineLayout m_computePipelineLayout;
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
