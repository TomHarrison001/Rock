/** \file engineApp.hpp */

#pragma once

#include <chrono>

#include <glm/gtc/constants.hpp>

#include "core/application.hpp"

class EngineApp : public Application
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
    void createDescriptorSetLayouts();
    void createGraphicsPipeline();
    void loadTexture(entt::entity entity, const char* path);
    void createTextureImageView(entt::entity entity);
    void createTextureSampler(entt::entity entity);
    void loadModel(entt::entity entity, const char* path);
    void createVertexBuffer(entt::entity entity);
    void createIndexBuffer(entt::entity entity);
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createTextureDescriptorSet(entt::entity entity);

    void updateUniformBuffer(uint32_t currentImage);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
public:
    void drawFrame() override;
private:
    Pipeline* m_graphicsPipeline;
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT; // multisample anti-aliasing

    // textures
    VkDescriptorSetLayout m_textureDescriptorSetLayout;

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

    entt::registry m_registry;
    entt::entity m_gameObject;
    entt::entity m_floor;
    float m_translate[3]{ 0.f, 0.f, 0.f };
    float m_rotate[3]{ 0.f, 0.f, 0.f };
    float m_scale[3]{ 1.f, 1.f, 1.f };
};
