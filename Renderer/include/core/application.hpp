/** \file application.hpp */

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "core/descriptors.hpp"
#include "rendering/renderer.hpp"

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

/* \class Application
*  \brief provides an application with a window, device, renderer and descriptor manager
*/
class Application
{
protected:
	virtual void initApplication() = 0; //!< virtual function to initialise the application
	virtual void mainLoop() = 0; //!< virtual function for the main loop
	virtual void cleanup() = 0; //!< virtual function to cleanup the application on destruction
public:
	virtual void run()
	{
		initApplication();
		mainLoop();
		cleanup();
	}; //!< function to run the application
	virtual void drawFrame() = 0; //!< virtual function to draw a frame
protected:
	Device* m_device; //!< pointer to the device object
	Renderer* m_renderer; //!< pointer to the renderer
	DescriptorManager* m_descriptorManager; //!< descriptor manager
};
