/** \file engineApp.hpp */

#pragma once

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#define IMGUI_ENABLE_FREETYPE  // higher quality font rendering

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

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
    bool m_showAnotherWindow = false;
    glm::vec3 m_clearColour = glm::vec3(0.45f, 0.55f, 0.6f);
};
