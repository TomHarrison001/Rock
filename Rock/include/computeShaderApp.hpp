/** \file computeShaderApp.hpp */

#pragma once

#include <random>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/application.hpp"
#include "rendering/pipeline.hpp"

struct UniformBufferObject {
	float u_deltaTime = 1.f;
};

class ComputeShaderApp : public Application
{
private:
	void initApplication() override;
	void mainLoop() override;
	void cleanup() override;
	void createDescriptorPool();
	void createGraphicsDescriptorSetLayout();
	void createComputeDescriptorSetLayout();
	void createGraphicsDescriptorSets();
	void createComputeDescriptorSets();
	void createRenderPipeline();
	void createComputePipeline();
	void createShaderStorageBuffers();
	void createUniformBuffers();
	void updateUniformBuffer(uint32_t currentImage);
public:
	void run() override;
	void drawFrame() override;
private:
	Pipeline* m_graphicsPipeline;
	Pipeline* m_computePipeline;
	const uint32_t m_particleCount = 8192;

	std::vector<VkBuffer> m_shaderStorageBuffers;
	std::vector<VkDeviceMemory> m_shaderStorageBuffersMemory;

	VkImageView m_textureImageView;
	VkSampler m_textureSampler;
	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;
	std::vector<void*> m_uniformBuffersMapped;

	float m_lastFrameTime = 0.f;
	double m_lastTime = 0.f;
};
