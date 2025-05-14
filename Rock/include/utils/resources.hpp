/** \file resources.hpp */

#pragma once

enum Resource { COLOUR, DEPTH }; //!< enums for resources
enum Stage { COMPUTE, GRAPHICS }; //!< enums for stages

#include <array>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

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
