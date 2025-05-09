#pragma once

#include "core/descriptors.hpp"
#include "rendering/renderer.hpp"

/* \class Application
*  \brief provides an application with a window, device, renderer and descriptor manager
*/
class Application
{
public:
	virtual void initApplication() = 0; //!< virtual function to initialise the application
	virtual void mainLoop() = 0; //!< virtual function for the main loop
	virtual void cleanup() = 0; //!< virtual function to cleanup the application on destruction
public:
	virtual void run() = 0; //!< virtual function to run the application
	virtual void drawFrame() = 0; //!< virtual function to draw a frame
protected:
	Device* m_device; //!< pointer to the device object
	Renderer* m_renderer; //!< pointer to the renderer
	DescriptorManager* m_descriptorManager; //!< descriptor manager
};
