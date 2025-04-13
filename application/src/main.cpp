#include <GLFW/glfw3.h>
#include <LVK.h>

int main()
{
    minilog::initialize(nullptr, { .threadNames = false });
    int width = 960;
    int height = 540;
    GLFWwindow* window = lvk::initWindow("Rock", width, height);

    std::unique_ptr<lvk::IContext> ctx = lvk::createVulkanContextWithSwapchain(window, width, height, {});

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glfwGetFramebufferSize(window, &width, &height);
        if (!width || !height) continue;
        lvk::ICommandBuffer& buf = ctx->acquireCommandBuffer();
        ctx->submit(buf, ctx->getCurrentSwapchainTexture());
    }

    ctx.reset();
    glfwDestroyWindow(window);
    glfwTerminate();

	return 0;
}
