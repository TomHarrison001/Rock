/** \file computeApp.hpp */

#pragma once

#include <random>

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include "core/application.hpp"

/* \struct Particle
*  \brief stores the data sent to the SSBO for each particle: position, velocity and colour; also handles binding and attribute descriptions
*/
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

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        VkVertexInputAttributeDescription posAttrib{};
        posAttrib.binding = 0;
        posAttrib.location = 0;
        posAttrib.format = VK_FORMAT_R32G32_SFLOAT;
        posAttrib.offset = offsetof(Particle, position);
        attributeDescriptions.push_back(posAttrib);

        VkVertexInputAttributeDescription colAttrib{};
        colAttrib.binding = 0;
        colAttrib.location = 1;
        colAttrib.format = VK_FORMAT_R32G32_SFLOAT;
        colAttrib.offset = offsetof(Particle, colour);
        attributeDescriptions.push_back(colAttrib);

        return attributeDescriptions;
    }
};

class ComputeApp : public Application
{
private:
	struct UniformBufferObject {
		float u_deltaTime = 1.f;
	};

	void initApplication() override;
	void mainLoop() override;
	void cleanup() override;
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createComputePipeline();
	void createShaderStorageBuffers();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void updateUniformBuffer(uint32_t currentImage);
public:
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
