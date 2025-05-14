/** \file modelApp.hpp */

#pragma once

#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "core/application.hpp"
#include "rendering/pipeline.hpp"

/* \struct Vertex
*  \brief stores the data sent to the SSBO for each vertex: position, tex coord and colour; also handles binding and attribute descriptions
*/
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 colour;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        VkVertexInputAttributeDescription posAttrib{};
        posAttrib.binding = 0;
        posAttrib.location = 0;
        posAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
        posAttrib.offset = offsetof(Vertex, pos);
        attributeDescriptions.push_back(posAttrib);

        VkVertexInputAttributeDescription colAttrib{};
        colAttrib.binding = 0;
        colAttrib.location = 1;
        colAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
        colAttrib.offset = offsetof(Vertex, colour);
        attributeDescriptions.push_back(colAttrib);

        VkVertexInputAttributeDescription texCoordAttrib{};
        texCoordAttrib.binding = 0;
        texCoordAttrib.location = 2;
        texCoordAttrib.format = VK_FORMAT_R32G32_SFLOAT;
        texCoordAttrib.offset = offsetof(Vertex, texCoord);
        attributeDescriptions.push_back(texCoordAttrib);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && colour == other.colour && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.colour) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

class ModelApp : public Application
{
private:
    // alignment requirements: https://docs.vulkan.org/spec/latest/chapters/interfaces.html#interfaces-resources-layout
    // scalars aligned - 4 bytes, vec2 aligned - 8 bytes, and vec3/vec4 aligned - 16 bytes
    // alignas specifies the alignment requirement of a type or object
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    void initApplication() override;
    void mainLoop() override;
    void cleanup() override;
    void createGraphicsDescriptorSetLayout();
    void createGraphicsPipeline();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void loadModel();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createGraphicsDescriptorSets();

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

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;
};
