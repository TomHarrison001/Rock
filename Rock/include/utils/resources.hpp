/** \file resources.hpp */

#pragma once

enum Resource { COLOUR, DEPTH }; //!< enums for resources
enum Stage { COMPUTE, GRAPHICS }; //!< enums for stages

#include <array>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

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

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Particle, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Particle, colour);

        return attributeDescriptions;
    }
};
