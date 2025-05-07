#pragma once

#include "rendering/swapchain.hpp"
#include "rendering/pipeline.hpp"

/* \class Renderer
*  \brief creates, records, submits and frees command buffers, maintains the swapchain during its lifecycle
*/
class Renderer
{
public:
	Renderer(Device* device); //!< constructor
	~Renderer(); //!< destructor

	Renderer(const Renderer&) = delete; //!< copy constructor
	Renderer& operator=(const Renderer&) = delete; //!< copy assignment

	uint32_t getCurrentFrame() { return m_currentFrame; } //!< returns the index of the current frame
	VkFence& getGraphicsInFlightFence() const { return m_swapchain->getGraphicsInFlightFence(m_currentFrame); } //!< returns the graphics in flight fence for the current frame from the swapchain
	VkFence& getComputeInFlightFence() const { return m_swapchain->getComputeInFlightFence(m_currentFrame); } //!< returns the compute in flight fence for the current frame from the swapchain
	VkCommandBuffer getGraphicsCommandBuffer() const { return m_graphicsCommandBuffers[m_currentFrame]; } //!< returns the graphics command buffer for the current frame
	VkCommandBuffer getComputeCommandBuffer() const { return m_computeCommandBuffers[m_currentFrame]; } //!< returns the compute command buffer for the current frame
	VkRenderPass getSwapchainRenderPass() const { return m_swapchain->getRenderPass(); } //!< returns the render pass from the swapchain
private:
	void createCommandBuffers(Stage stage); //!< creates the command buffers for the input stage
	void freeCommandBuffers(); //!< frees all command buffers from m_graphicsCommandBuffers and m_computeCommandBuffers
	void recreateSwapchain(); //!< recreates the swapchain when the extents change or window is resized
public:
	void beginFrame(); //!< acquires the next swapchain image
	void endFrame(); //!< queues the retrieved image for rendering
	void beginSwapchainRenderPass(VkCommandBuffer commandBuffer); //!< sets the render pass info before beginning the pass
	void endSwapchainRenderPass(VkCommandBuffer commandBuffer); //!< ends the swap chain render pass
	void recordCommandBuffer(Stage stage, Pipeline* pipeline, const uint32_t m_particleCount = 0, std::vector<VkBuffer> shaderStorageBuffers = {}, std::vector<VkDescriptorSet> descriptorSets = {}); //!< begins the current command buffer, binds the relevant pipeline, calls vkDraw or vkDispatch and ends the command buffer
	void submitCommandBuffer(Stage stage); //!< submits the current command buffer to a device queue
private:
	Window* m_window; //!< window object pointer
	Device* m_device; //!< device object pointer
	Swapchain* m_swapchain; //!< pointer to active swapchain
	std::vector<VkCommandBuffer> m_graphicsCommandBuffers; //!< vector for graphics command buffers
	std::vector<VkCommandBuffer> m_computeCommandBuffers; //!< vector for compute command buffers

	uint32_t m_imageIndex; //!< index of next image for present info and framebuffer index
	uint32_t m_currentFrame = 0; //!< stores the current frame
};
