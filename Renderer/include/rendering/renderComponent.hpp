#pragma once

#include "core/device.hpp"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

/* \struct Vertex
*  \brief stores the data sent to the SSBO for each vertex: position, tex coord and colour; also handles binding and attribute descriptions
*/
struct Vertex
{
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 norm;

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

        VkVertexInputAttributeDescription texCoordAttrib{};
        texCoordAttrib.binding = 0;
        texCoordAttrib.location = 1;
        texCoordAttrib.format = VK_FORMAT_R32G32_SFLOAT;
        texCoordAttrib.offset = offsetof(Vertex, texCoord);
        attributeDescriptions.push_back(texCoordAttrib);

        VkVertexInputAttributeDescription normAttrib{};
        normAttrib.binding = 0;
        normAttrib.location = 2;
        normAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
        normAttrib.offset = offsetof(Vertex, norm);
        attributeDescriptions.push_back(normAttrib);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && texCoord == other.texCoord && norm == other.norm;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.norm) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

namespace Rock
{
    struct RenderComponent
    {
        // texture
        VkDescriptorSet descriptorSet;
        uint32_t m_mipLevels;
        VkImage m_textureImage;
        VkDeviceMemory m_textureImageMemory;
        VkImageView m_textureImageView;
        VkSampler m_textureSampler;

        // model
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        VkBuffer m_vertexBuffer;
        VkBuffer m_indexBuffer;
        VkDeviceMemory m_vertexBufferMemory;
        VkDeviceMemory m_indexBufferMemory;
    };
}
