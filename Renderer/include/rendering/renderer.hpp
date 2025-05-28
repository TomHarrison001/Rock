/** \file renderer.hpp */

#pragma once

#include "rendering/lights.hpp"
#include "rendering/swapchain.hpp"
#include "rendering/pipeline.hpp"
#include "rendering/renderComponent.hpp"
#include "window/ui.hpp"

#include <entt/entt.hpp>

#include "components/transformComponent.hpp"
#include "components/colliderComponent.hpp"
#include "components/rigidbodyComponent.hpp"

/* \class Renderer
*  \brief creates, records, submits and frees command buffers, maintains the swapchain during its lifecycle
*/
class Renderer
{
public:
	Renderer(Device* device, VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT, bool resources = false); //!< constructor
	~Renderer(); //!< destructor

	Renderer(const Renderer&) = delete; //!< copy constructor
	Renderer& operator=(const Renderer&) = delete; //!< copy assignment

	uint32_t getCurrentFrame() { return m_currentFrame; } //!< returns the index of the current frame
	void setCurrentFrame(uint32_t newFrame) { m_currentFrame = newFrame; } //!< set the index of the current frame
	VkSwapchainKHR getSwapchain() { return m_swapchain->getSwapchain(); } //!< returns the current swapchain
	VkSemaphore& getImageAvailableSemaphore() const { return m_swapchain->getImageAvailableSemaphore(m_currentFrame); } //!< returns the image available semaphore for the current frame from the swapchain
	VkSemaphore& getGraphicsFinishedSemaphore() const { return m_swapchain->getGraphicsFinishedSemaphore(m_currentFrame); } //!< returns the graphics semaphore for the current frame from the swapchain
	VkFence& getFence() const { return m_swapchain->getFence(m_currentFrame); } //!< returns the in flight fence for the current frame from the swapchain
	VkCommandBuffer getCommandBuffer() const { return m_commandBuffers[m_currentFrame]; } //!< returns the graphics command buffer for the current frame
	VkRenderPass getSwapchainRenderPass() const { return m_swapchain->getRenderPass(); } //!< returns the render pass from the swapchain
	VkFramebuffer getSwapchainFramebuffer() const { return m_swapchain->getFramebuffer(m_currentFrame); } //!< returns the swapchain framebuffer for the current frame
	float getSwapchainAspectRatio() const { return static_cast<float>(m_swapchain->getSwapchainExtent().width) / static_cast<float>(m_swapchain->getSwapchainExtent().height); } //!< calculates and returns swapchain aspect ratio
	VkExtent2D getSwapchainExtent() const { return m_swapchain->getSwapchainExtent(); } //!< returns the swapchain extent
	uint32_t getSwapchainImageCount() const { return m_swapchain->getImageCount(); } //!< returns the swapchain image count
private:
	void recreateSwapchain(); //!< recreates the swapchain when the extents change or window is resized
	void createCommandBuffers(); //!< creates the command buffers for graphics and compute
public:
	void beginFrame(); //!< acquires the next swapchain image
	void endFrame(); //!< queues the retrieved image for rendering
	void beginSwapchainRenderPass(Pipeline* pipeline, VkCommandBuffer commandBuffer, bool depth = false); //!< sets the render pass info before beginning the pass
	void beginSwapchainRenderPass(VkClearValue& clearColour); //!< sets the render pass info before beginning the pass
	void recordCommandBuffer(bool compute, Pipeline* pipeline, const uint32_t m_particleCount = 0, std::vector<VkBuffer> shaderStorageBuffers = {}, std::vector<VkDescriptorSet> descriptorSets = {}); //!< begins the current command buffer, binds the relevant pipeline, calls vkDraw or vkDispatch and ends the command buffer
	void recordCommandBuffer(Pipeline* pipeline, entt::registry& m_registry, std::vector<entt::entity> entities, std::vector<VkDescriptorSet> descriptorSets); //!< begins the current command buffer, binds the relevant pipeline, calls vkDraw or vkDispatch and ends the command buffer
	void recordCommandBuffer(Pipeline* pipeline, VkBuffer vertexBuffer, VkBuffer indexBuffer, std::vector<VkDescriptorSet> descriptorSets, std::vector<uint32_t> indices, float* m_translation, float* m_rotation, float* m_scale); //!< begins the current command buffer, binds the relevant pipeline, calls vkDraw or vkDispatch and ends the command buffer
	void submitCommandBuffer(bool compute); //!< submits the current command buffer to a device queue
	void submitCommandBuffer(); //!< submits the current command buffer to a device queue
private:
	Window* m_window; //!< window object pointer
	Device* m_device; //!< device object pointer
	Swapchain* m_swapchain; //!< pointer to active swapchain
	std::vector<VkCommandBuffer> m_commandBuffers; //!< vector for command buffers

	uint32_t m_imageIndex; //!< index of next image for present info and framebuffer index
	uint32_t m_currentFrame = 0; //!< stores the current frame
	VkSampleCountFlagBits m_msaaSamples; // multisample anti-aliasing
	bool m_resources; //!< if the swapchain should create resources
};
