/** \file engineApp.hpp */

#pragma once

#include "window/ui.hpp"
#include "core/application.hpp"

class EngineApp : public Application
{
private:
    void initApplication() override;
    void mainLoop() override;
    void cleanup() override;
public:
    void drawFrame() override;
private:
    void createDescriptorPool();
    void initialiseImgui();
    void frameRender(ImDrawData* drawData);
    void framePresent();
private:
    bool m_showDemoWindow = true;
    glm::vec3 m_clearColour = glm::vec3(0.45f, 0.55f, 0.6f);
};
