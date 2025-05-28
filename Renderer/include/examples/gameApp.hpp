/** \file gameApp.hpp */

#pragma once

#include <chrono>

#include <glm/gtc/constants.hpp>

#include "core/application.hpp"

class GameApp : public Application
{
private:
    struct CameraUBO {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };
    struct LightUBO {
        alignas(16) DirectionalLight dLight;
        alignas(16) PointLight pLights[1];
    };
    struct ViewUBO {
        alignas(16) glm::vec3 viewPos;
    };

    void initApplication() override;
    void mainLoop() override;
    void cleanup() override;
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void loadModel(const char* path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void createVertexBuffer(std::vector<Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory);
    void createIndexBuffer(std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory);
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void updateUniformBuffer(uint32_t currentImage);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
public:
    void drawFrame() override;
private:
    Pipeline* m_graphicsPipeline;
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT; // multisample anti-aliasing

    uint32_t m_mipLevels;
    VkImage m_textureImage;
    VkDeviceMemory m_textureImageMemory;
    VkImageView m_textureImageView;
    VkSampler m_textureSampler;

    // camera and light UBOs
    std::vector<VkBuffer> m_cameraBuffers;
    std::vector<VkBuffer> m_lightBuffers;
    std::vector<VkBuffer> m_viewPosBuffers;
    std::vector<VkDeviceMemory> m_cameraBuffersMemory;
    std::vector<VkDeviceMemory> m_lightBuffersMemory;
    std::vector<VkDeviceMemory> m_viewPosBuffersMemory;
    std::vector<void*> m_cameraBuffersMapped;
    std::vector<void*> m_lightBuffersMapped;
    std::vector<void*> m_viewPosBuffersMapped;

    // cube 1
    std::vector<Vertex> m_cubeVertices;
    std::vector<uint32_t> m_cubeIndices;
    VkBuffer m_cubeVertexBuffer;
    VkBuffer m_cubeIndexBuffer;
    VkDeviceMemory m_cubeVertexBufferMemory;
    VkDeviceMemory m_cubeIndexBufferMemory;

    // cube 2
    std::vector<Vertex> m_cubeVertices2;
    std::vector<uint32_t> m_cubeIndices2;
    VkBuffer m_cubeVertexBuffer2;
    VkBuffer m_cubeIndexBuffer2;
    VkDeviceMemory m_cubeVertexBufferMemory2;
    VkDeviceMemory m_cubeIndexBufferMemory2;

    float m_position = 0.f;
};
