/** \file engineApp.cpp */

#include "examples/engineApp.hpp"

static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[Vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void EngineApp::initApplication()
{
    WindowSettings settings = WindowSettings();
    settings.width = 1280;
    settings.height = 720;
    //settings.resizable = false;
    settings.imguiEnabled = true;
    Window* window = new Window(settings);
    m_device = new Device(window);
    window = nullptr;
    m_descriptorManager = new DescriptorManager(m_device, Swapchain::MAX_FRAMES_IN_FLIGHT);
    
    createDescriptorPool();

    m_renderer = new Renderer(m_device);

    initialiseImgui();
}

void EngineApp::mainLoop()
{
    while (!m_device->getWindow()->shouldClose())
    {
        glfwPollEvents();
        drawFrame();

        if (m_device->isKeyPressed(GLFW_KEY_ESCAPE))
        {
            std::cout << "[EventSystem] Exiting..." << std::endl;
            m_device->closeWindow();
        }
    }

    vkDeviceWaitIdle(m_device->getDevice());
}

void EngineApp::cleanup()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete m_descriptorManager;
    m_descriptorManager = nullptr;
    delete m_renderer;
    m_renderer = nullptr;
    delete m_device;
    m_device = nullptr;
}

void EngineApp::drawFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    /****************************
    *     MainMenuBar           *
    ****************************/

    createMainMenuBar(m_device);

    /****************************
    *     FpsCounter            *
    ****************************/

    createOverlay(io.Framerate);

    /****************************
    *     Dockspace             *
    ****************************/

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    /****************************
    *     Viewport              *
    ****************************/

    ImGui::Begin("Viewport");
    ImGui::End();

    /****************************
    *     Editor                *
    ****************************/

    ImGui::Begin("Editor");
    ImGui::Text("main.cpp");
    ImGui::Text("");
    ImGui::Text("#include <iostream>");
    ImGui::Text("");
    ImGui::Text("int main(int argc, char* argv[])");
    ImGui::Text("    return 0;");
    ImGui::Text("}");
    ImGui::Text("");
    ImGui::End();

    /****************************
    *     Scene hierarchy       *
    ****************************/

    ImGui::Begin("Scene");
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Scene"))
    {
        for (int i = 0; i < 5; i++)
        {
            ImGui::PushID(i);
            if (ImGui::TreeNode("", "GameObject %d", i + 1))
            {
                ImGui::Text("Transform");
                ImGui::SameLine();
                if (ImGui::SmallButton("Edit")) {}
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    ImGui::End();

    /****************************
    *     File manager          *
    ****************************/

    ImGui::Begin("File Manager");
    ImGui::Text("File manager");
    ImGui::End();

    /****************************
    *     Properties            *
    ****************************/

    ImGui::Begin("Properties");
    ImGui::Text("GameObject 1");
    static float translation[3] = { 0.f, 0.f, 0.f };
    ImGui::DragFloat3("Translation", translation, 0.5f, -50.f, 50.f);
    static float rotation[3] = { 0.f, 0.f, 0.f };
    ImGui::DragFloat3("Rotation", rotation, 1.f, -180.f, 180.f);
    static float scale[3] = { 1.f, 1.f, 1.f };
    ImGui::DragFloat3("Scale", scale, 1.f, 0.f, 10.f);
    ImGui::End();

    /****************************
    *     Console               *
    ****************************/

    static Console console;
    console.draw("Console");

    //// show demo window
    //if (m_showDemoWindow)
    //    ImGui::ShowDemoWindow(&m_showDemoWindow);
    //
    //// show custom window
    //{
    //    static float f = 0.f;
    //    static int counter = 0;
    //
    //    ImGui::Begin("Hello imgui"); // create window called hello imgui
    //    ImGui::Text("Some useful text"); // display text
    //    ImGui::Checkbox("Demo", &m_showDemoWindow);
    //    ImGui::SliderFloat("float", &f, 0.f, 1.f);
    //    ImGui::ColorEdit3("Clear colour", (float*)&m_clearColour);
    //
    //    if (ImGui::Button("Button"))
    //        counter++;
    //    ImGui::SameLine();
    //    ImGui::Text("Counter: %d", counter);
    //    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.f / io.Framerate, io.Framerate);
    //    ImGui::End();
    //}

    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    const bool minimised = (drawData->DisplaySize.x <= 0.f || drawData->DisplaySize.y <= 0.f);
    if (!minimised)
        frameRender(drawData);

    // update and render additional platform windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    // present main platform window
    if (!minimised)
        framePresent();
}

void EngineApp::createDescriptorPool()
{
    m_descriptorManager->addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(Swapchain::MAX_FRAMES_IN_FLIGHT));
    m_descriptorManager->buildDescriptorPool();
}

void EngineApp::initialiseImgui()
{
    // setup dear imgui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // enable keyboard controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // enable gamepad controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // enable docking
    io.ConfigDockingWithShift = false; // hold shift to disable docking instead of hold shift to enable docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // enable multi-viewport / platform windows
    
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // load font
    io.Fonts->AddFontFromFileTTF("./res/fonts/Segoe-UI-Variable-Static-Text-Semibold.ttf", 16.f);

    // setup platform/renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_device->getWindow()->getWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_3;
    init_info.Instance = m_device->getInstance();
    init_info.PhysicalDevice = m_device->getPhysicalDevice();
    init_info.Device = m_device->getDevice();
    init_info.QueueFamily = m_device->findPhysicalQueueFamilies().graphicsFamily.value();
    init_info.Queue = m_device->getGraphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_descriptorManager->getDescriptorPool();
    init_info.RenderPass = m_renderer->getSwapchainRenderPass();
    init_info.Subpass = 0;
    init_info.MinImageCount = Swapchain::MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = m_renderer->getSwapchainImageCount();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);
}

void EngineApp::frameRender(ImDrawData* drawData)
{
    vkWaitForFences(m_device->getDevice(), 1, &m_renderer->getFence(), VK_TRUE, UINT64_MAX);
    m_renderer->beginFrame();
    vkResetFences(m_device->getDevice(), 1, &m_renderer->getFence());
    vkResetCommandBuffer(m_renderer->getCommandBuffer(), 0);

    {
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(m_renderer->getCommandBuffer(), &info);
    }

    {
        VkClearValue clearColour;
        clearColour.color.float32[0] = m_clearColour.x;
        clearColour.color.float32[1] = m_clearColour.y;
        clearColour.color.float32[2] = m_clearColour.z;
        clearColour.color.float32[3] = 1.f;
        m_renderer->beginSwapchainRenderPass(clearColour);
    }

    // record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(drawData, m_renderer->getCommandBuffer());

    // Submit command buffer
    vkCmdEndRenderPass(m_renderer->getCommandBuffer());

    {
        VkCommandBuffer commandBuffer = m_renderer->getCommandBuffer();
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &m_renderer->getImageAvailableSemaphore();
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &commandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &m_renderer->getGraphicsFinishedSemaphore();

        vkEndCommandBuffer(m_renderer->getCommandBuffer());
        vkQueueSubmit(m_device->getGraphicsQueue(), 1, &info, m_renderer->getFence());
    }
}

void EngineApp::framePresent()
{
    VkSemaphore graphicsFinishedSemamphore = m_renderer->getGraphicsFinishedSemaphore();
    VkSwapchainKHR swapchain = m_renderer->getSwapchain();
    uint32_t currentFrame = m_renderer->getCurrentFrame();
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &graphicsFinishedSemamphore;
    info.swapchainCount = 1;
    info.pSwapchains = &swapchain;
    info.pImageIndices = &currentFrame;
    VkResult err = vkQueuePresentKHR(m_device->getPresentQueue(), &info);
    m_renderer->setCurrentFrame((m_renderer->getCurrentFrame() + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT);
}
