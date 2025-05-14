/** \file renderer.cpp */

#include "rendering/renderer.hpp"

Renderer::Renderer(Device* device, VkSampleCountFlagBits msaaSamples, bool resources)
	: m_device(device), m_msaaSamples(msaaSamples), m_resources(resources)
{
    m_window = m_device->getWindow();
	recreateSwapchain();
	createCommandBuffers(Stage::GRAPHICS);
	createCommandBuffers(Stage::COMPUTE);
}

Renderer::~Renderer()
{
	freeCommandBuffers();
    delete m_swapchain;
    m_swapchain = nullptr;
	m_device = nullptr;
	m_window = nullptr;
}

void Renderer::createCommandBuffers(Stage stage)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_device->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT);

    if (stage == Stage::GRAPHICS)
    {
        m_graphicsCommandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateCommandBuffers(m_device->getDevice(), &allocInfo, m_graphicsCommandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate graphics command buffers.");
    }
    else if (stage == Stage::COMPUTE)
    {
        m_computeCommandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateCommandBuffers(m_device->getDevice(), &allocInfo, m_computeCommandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate compute command buffers.");
    }
    else
        throw std::runtime_error("Unsupported pipeline stage used to create command buffers.");
}

void Renderer::freeCommandBuffers()
{
    vkFreeCommandBuffers(m_device->getDevice(), m_device->getCommandPool(), static_cast<uint32_t>(m_graphicsCommandBuffers.size()), m_graphicsCommandBuffers.data());
    m_graphicsCommandBuffers.clear();
    vkFreeCommandBuffers(m_device->getDevice(), m_device->getCommandPool(), static_cast<uint32_t>(m_computeCommandBuffers.size()), m_computeCommandBuffers.data());
    m_computeCommandBuffers.clear();
}

void Renderer::recreateSwapchain()
{
    VkExtent2D extent = m_window->getExtent();
    while (extent.width == 0 || extent.height == 0)
    {
        extent = m_window->getExtent();
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_device->getDevice());

    if (m_swapchain != nullptr)
    {
        Swapchain* oldSwapchain = std::move(m_swapchain);
        m_swapchain = new Swapchain(m_device, oldSwapchain, m_msaaSamples);
        if (*oldSwapchain != m_swapchain)
            throw std::runtime_error("Swap chain image/depth format has changed.");
        delete oldSwapchain;
        oldSwapchain = nullptr;
    }
    else
        m_swapchain = new Swapchain(m_device, m_msaaSamples, m_resources);
}

void Renderer::beginFrame()
{
    VkResult result = vkAcquireNextImageKHR(m_device->getDevice(), m_swapchain->getSwapchain(), UINT64_MAX, m_swapchain->getImageAvailableSemaphore(m_currentFrame), VK_NULL_HANDLE, &m_imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swap chain image.");
}

void Renderer::endFrame()
{
    VkSemaphore waitSemaphores[] = { m_swapchain->getGraphicsFinishedSemaphore(m_currentFrame) };

    VkSwapchainKHR swapchains[] = { m_swapchain->getSwapchain() };
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &m_imageIndex;
    presentInfo.pResults = nullptr; // optional

    VkResult result = vkQueuePresentKHR(m_device->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_device->getWindow()->getResized())
    {
        m_device->getWindow()->setResized(false);
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to present swap chain image.");

    m_currentFrame = (m_currentFrame + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::beginSwapchainRenderPass(VkCommandBuffer commandBuffer, bool depth)
{
    std::vector<VkClearValue> clearColours = { {{ 0.f, 0.f, 0.f, 1.f }} };
    if (depth) clearColours.push_back({ {{1.f, 0}} });
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_swapchain->getRenderPass();
    renderPassInfo.framebuffer = m_swapchain->getFramebuffer(m_imageIndex);
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapchain->getSwapchainExtent();
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColours.size());
    renderPassInfo.pClearValues = clearColours.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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
}

void Renderer::endSwapchainRenderPass(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}

void Renderer::recordCommandBuffer(Stage stage, Pipeline* pipeline, const uint32_t m_particleCount, std::vector<VkBuffer> shaderStorageBuffers, std::vector<VkDescriptorSet> descriptorSets)
{
    VkCommandBuffer commandBuffer;
    if (stage == Stage::GRAPHICS) commandBuffer = m_graphicsCommandBuffers[m_currentFrame];
    else if (stage == Stage::COMPUTE) commandBuffer = m_computeCommandBuffers[m_currentFrame];
    else throw std::runtime_error("Unsupported stage used to record command buffer.");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // optional
    beginInfo.pInheritanceInfo = nullptr; // optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer.");

    if (stage == Stage::GRAPHICS)
    {
        pipeline->bindGraphics(commandBuffer);
        beginSwapchainRenderPass(commandBuffer);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shaderStorageBuffers[m_currentFrame], offsets);
        vkCmdDraw(commandBuffer, m_particleCount, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
    }
    if (stage == Stage::COMPUTE)
    {
        pipeline->bindCompute(commandBuffer);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getPipelineLayout(), 0, 1, &descriptorSets[m_currentFrame], 0, nullptr);
        vkCmdDispatch(commandBuffer, m_particleCount / 256, 1, 1);
    }

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record render command buffer.");
}

void Renderer::recordCommandBuffer(Pipeline* pipeline, VkBuffer vertexBuffer, VkBuffer indexBuffer, std::vector<VkDescriptorSet> descriptorSets, std::vector<uint32_t> indices)
{
    VkCommandBuffer commandBuffer = m_graphicsCommandBuffers[m_currentFrame];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // optional
    beginInfo.pInheritanceInfo = nullptr; // optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer.");

    pipeline->bindGraphics(commandBuffer);
    beginSwapchainRenderPass(commandBuffer, true);

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(), 0, 1, &descriptorSets[m_currentFrame], 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record render command buffer.");
}

void Renderer::submitCommandBuffer(Stage stage)
{
    VkCommandBuffer commandBuffer;
    if (stage == Stage::GRAPHICS) commandBuffer = m_graphicsCommandBuffers[m_currentFrame];
    else if (stage == Stage::COMPUTE) commandBuffer = m_computeCommandBuffers[m_currentFrame];
    else throw std::runtime_error("Unsupported stage used to record command buffer.");

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    if (stage == Stage::GRAPHICS)
    {
        VkSemaphore waitSemaphores[] = { m_swapchain->getComputeFinishedSemaphore(m_currentFrame), m_swapchain->getImageAvailableSemaphore(m_currentFrame) };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        submitInfo.waitSemaphoreCount = 2;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
    }

    VkSemaphore semaphore = (stage == Stage::GRAPHICS) ? m_swapchain->getGraphicsFinishedSemaphore(m_currentFrame) : m_swapchain->getComputeFinishedSemaphore(m_currentFrame);
    VkQueue queue = (stage == Stage::GRAPHICS) ? m_device->getGraphicsQueue() : m_device->getComputeQueue();
    VkFence fence = (stage == Stage::GRAPHICS) ? m_swapchain->getGraphicsInFlightFence(m_currentFrame) : m_swapchain->getComputeInFlightFence(m_currentFrame);

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphore;

    if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit command buffer.");
}

void Renderer::submitCommandBuffer()
{
    VkCommandBuffer commandBuffer = m_graphicsCommandBuffers[m_currentFrame];

    VkSemaphore waitSemaphores[] = { m_swapchain->getImageAvailableSemaphore(m_currentFrame) };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { m_swapchain->getGraphicsFinishedSemaphore(m_currentFrame) };

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, m_swapchain->getGraphicsInFlightFence(m_currentFrame)) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit command buffer.");
}
