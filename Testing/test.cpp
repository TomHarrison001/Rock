#include "pch.h"
#include "test.h"

// variables for Physics testing
entt::registry m_entities;
entt::entity gameObject{ entt::null };
entt::entity obb1{ entt::null };
entt::entity obb2{ entt::null };
entt::entity sphere1{ entt::null };
entt::entity sphere2{ entt::null };

// variables for Renderer testing
const int WIDTH = 1280;
const int HEIGHT = 720;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> instanceExtensions = {
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

GLFWwindow* m_window;
VkInstance m_instance;
VkSurfaceKHR m_surface;
VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
VkDevice m_device;
VkQueue m_queue;
VkCommandPool m_commandPool;
VkSwapchainKHR m_swapchain;
VkFormat m_swapchainImageFormat;
VkFormat m_swapchainDepthFormat;
VkExtent2D m_swapchainExtent;
VkRenderPass m_renderPass;
std::vector<VkImageView> m_swapchainImageViews;
std::vector<VkImage> m_swapchainImages;
std::vector<VkDeviceMemory> m_swapchainImageMemory;
std::vector<VkFramebuffer> m_framebuffers;
std::vector<VkSemaphore> m_semaphores;
std::vector<VkFence> m_fences;

Test::~Test()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_device, m_semaphores[i], nullptr);
        vkDestroyFence(m_device, m_fences[i], nullptr);
    }
    m_semaphores.clear();
    m_fences.clear();
    for (auto framebuffer : m_framebuffers)
    {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    m_framebuffers.clear();
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    for (int i = 0; i < m_swapchainImages.size(); i++)
    {
        vkDestroyImageView(m_device, m_swapchainImageViews[i], nullptr);
        //vkDestroyImage(m_device, m_swapchainImages[i], nullptr);
        //vkFreeMemory(m_device, m_swapchainImageMemory[i], nullptr);
    }
    m_swapchainImageViews.clear();
    m_swapchainImages.clear();
    m_swapchainImageMemory.clear();
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

int Test::RunTests(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

TEST(PhysicsEngine, TestClamp)
{
    int i = 5;
    i = Rock::clamp(i, 4, 4);
    ASSERT_EQ(i, 4);
    ASSERT_ANY_THROW(Rock::clamp(i, 10, 0));
    i = Rock::clamp(i, 6, 10);
    ASSERT_EQ(i, 6);
    i = Rock::clamp(i, 0, 4);
    ASSERT_EQ(i, 4);

    float f = 5.f;
    f = Rock::clamp(f, 4.f, 4.f);
    ASSERT_EQ(f, 4.f);
    ASSERT_ANY_THROW(Rock::clamp(f, 10.f, 0.f));
    f = Rock::clamp(f, 6.f, 10.f);
    ASSERT_EQ(f, 6.f);
    f = Rock::clamp(f, 0.f, 4.f);
    ASSERT_EQ(f, 4.f);

    Rock::Vector3 v(1.f, 1.f, 1.f);
    v.clamp(0.f, 0.f);
    ASSERT_EQ(v, Rock::Vector3(0.f, 0.f, 0.f));
    ASSERT_ANY_THROW(v.clamp(10.f, 0.f));
    v = Rock::Vector3(0.f, 5.f, 10.f);
    v.clamp(4.f, 10.f);
    ASSERT_EQ(v, Rock::Vector3(4.f, 5.f, 10.f));
    v.clamp(0.f, 6.f);
    ASSERT_EQ(v, Rock::Vector3(4.f, 5.f, 6.f));

    v = Rock::Vector3(0.f, 5.f, 10.f);
    Rock::Vector3 min(10.f, 0.f, -10.f);
    Rock::Vector3 max(10.f, 4.f, 0.f);
    v.clamp(min, max);
    ASSERT_EQ(v, Rock::Vector3(10.f, 4.f, 0.f));
}

TEST(PhysicsEngine, TestSUVATeqns)
{
    glm::vec3 s, u, v, a;
    float t;
    // an object has the following properties: u = 2i + 3j + 4k, a = 4i + 2j + 3k. Find v after 2 and 4 seconds.
    u = glm::vec3(2.f, 3.f, 4.f);
    a = glm::vec3(4.f, 2.f, 3.f);
    t = 2.f;
    v = Rock::calculateVelocity(u, a, t);
    ASSERT_EQ(v, glm::vec3(10.f, 7.f, 10.f));
    t = 4.f;
    v = Rock::calculateVelocity(u, a, t);
    ASSERT_EQ(v, glm::vec3(18.f, 11.f, 16.f));

    // a cannonball is released from a cannon at ground level with an initial speed of 250m per second.
    // it is fired at an angle of 30 degrees to the ground. find how far it travels horizontally from
    // the firing point before it explodes if it is designed to explode 10m above the ground on descent.
    // assume gravity is 10 m/s
    s = glm::vec3(0.f, 10.f, 0.f);
    u = glm::vec3(250.f * cos(30.f * 3.14159f / 180.f), 250.f * sin(30.f * 3.14159f / 180.f), 0.f);
    a = glm::vec3(0.f, -10.f, 0.f);
    std::tuple<float, float> times = Rock::calculateTime(u.y, a.y, s.y);
    t = std::max(std::get<0>(times), std::get<1>(times));
    v.x = 250 * cos(30 * 3.14159 / 180);
    s.x = (u.x + v.x) / 2 * t;
    ASSERT_EQ(floor(s.x), 5395);
}

TEST(PhysicsEngine, TestVector2)
{
    Rock::Vector2 v1(1.f, 1.f);
    Rock::Vector2 v2(4.f, 2.f);
    Rock::Vector2 result(0.f, 0.f);

    result = -v1;
    ASSERT_EQ(result, Rock::Vector2(-1.f, -1.f));

    result = v1 - v2;
    ASSERT_EQ(result, Rock::Vector2(-3.f, -1.f));

    result = v1 + v2;
    ASSERT_EQ(result, Rock::Vector2(5.f, 3.f));

    result = v1 * v2;
    ASSERT_EQ(result, Rock::Vector2(4.f, 2.f));

    float n = 5.f;
    result = v1 * n;
    ASSERT_EQ(result, Rock::Vector2(5.f, 5.f));

    result = v1 / v2;
    ASSERT_EQ(result, Rock::Vector2(0.25f, 0.5f));

    result = v1 / n;
    ASSERT_EQ(result, Rock::Vector2(0.2f, 0.2f));

    ASSERT_TRUE(v1 == v1);
    ASSERT_FALSE(v1 == v2);

    v1 = Rock::Vector2(3.f, 4.f);
    v2 = Rock::Vector2(7.f, 7.f);

    ASSERT_EQ(v1.length(), 5.f);
    ASSERT_EQ(v1.normalise(), Rock::Vector2(0.6f, 0.8f));
    ASSERT_EQ(v1.distance(v2), 5.f);
    ASSERT_EQ(v1.dot(v2), 49.f);
}

TEST(PhysicsEngine, TestVector3)
{
    Rock::Vector3 v1(3.f, 4.f, 0.f);
    Rock::Vector3 v2(7.f, 7.f, 0.f);
    
    ASSERT_EQ(v1.length(), 5.f);
    ASSERT_EQ(v1.normalise(), Rock::Vector3(0.6f, 0.8f, 0.f));
    ASSERT_EQ(v1.distance(v2), 5.f);
    ASSERT_EQ(v1.dot(v2), 49.f);
    ASSERT_EQ(v1.cross(v2), Rock::Vector3(0.f, 0.f, -7.f));
}

TEST(PhysicsEngine, TestMatrix2)
{
    Rock::Matrix2 m1(3);
    Rock::Matrix2 m2(3, 2, 1, 0);

    m1 -= m2;

    ASSERT_EQ(m1.getRow(0), Rock::Vector2(0, 1));
    ASSERT_EQ(m1.getRow(1), Rock::Vector2(2, 3));
    ASSERT_EQ(m1.getColumn(0), Rock::Vector2(0, 2));
    ASSERT_EQ(m1.getColumn(1), Rock::Vector2(1, 3));

    m1 = m1 * m2;

    ASSERT_EQ(m1.getRow(0), Rock::Vector2(1, 0));
    ASSERT_EQ(m1.getRow(1), Rock::Vector2(9, 4));

    ASSERT_EQ(m1.getRow(0), m1.getTranspose().getColumn(0));
    ASSERT_EQ(m1.getRow(1), m1.getTranspose().getColumn(1));
}

TEST(PhysicsEngine, TestMatrix3)
{
    Rock::Matrix3 m1(1);
    Rock::Matrix3 m2, m1m2, m2m1;
    m2 = Rock::Matrix3(1, 2, 3,
                       2, 3, 4,
                       3, 4, 5);

    ASSERT_TRUE(m2 == m2.getTranspose());

    m1m2 = Rock::Matrix3(6, 9, 12,
                         6, 9, 12,
                         6, 9, 12);
    m2m1 = Rock::Matrix3(6, 6, 6,
                         9, 9, 9,
                         12,12,12);

    ASSERT_TRUE(m1 * m2 == m1m2);
    ASSERT_TRUE(m2 * m1 == m2m1);

    ASSERT_EQ(m1.getDeterminant(), 0.f);
}

TEST(PhysicsEngine, TestTransformComponent)
{
    gameObject = m_entities.create();

    auto& transformComp = m_entities.emplace<Rock::TransformComponent>(gameObject,
        glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f));
    
    ASSERT_EQ(transformComp.m_translation, glm::vec3(0.f));
    ASSERT_EQ(transformComp.m_rotation, glm::quat(glm::vec3(0.f)));
    ASSERT_EQ(transformComp.m_scale, glm::vec3(1.f));
    
    // test recalculate
    glm::mat4 t = glm::translate(glm::mat4(1.f), transformComp.m_translation);
    glm::mat4 r = glm::toMat4(transformComp.m_rotation);
    glm::mat4 s = glm::scale(glm::mat4(1.f), transformComp.m_scale);
    ASSERT_EQ(transformComp.m_transform, t * r * s);
    
    // change translation, rotation and scale
    transformComp.m_translation += glm::vec3(100.f, 100.f, 0.f);
    transformComp.m_rotation += glm::quat(glm::vec3(100.f, 100.f, 0.f));
    transformComp.m_scale *= glm::vec3(2.f, 2.f, 2.f);
    transformComp.recalculate();
    
    // test recalculate
    t = glm::translate(glm::mat4(1.f), glm::vec3(100.f, 100.f, 0.f));
    r = glm::toMat4(glm::quat(glm::vec3(0.f)) + glm::quat(glm::vec3(100.f, 100.f, 0.f)));
    s = glm::scale(glm::mat4(1.f), glm::vec3(2.f, 2.f, 2.f));
    ASSERT_EQ(transformComp.m_transform, t * r * s);
}

TEST(PhysicsEngine, TestColliderComponent)
{
    obb1 = m_entities.create();
    obb2 = m_entities.create();
    sphere2 = m_entities.create();
    sphere2 = m_entities.create();
    
    for (entt::entity obb : {obb1, obb2})
    {
        auto& transformComp = m_entities.emplace<Rock::TransformComponent>(obb,
            glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f));
        auto& collisionComp = m_entities.emplace<Rock::OBBComponent>(obb, glm::vec3(0.5f));
    }

    for (entt::entity sphere : {sphere1, sphere2})
    {
        auto& transformComp = m_entities.emplace<Rock::TransformComponent>(sphere,
            glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f));
        auto& collisionComp = m_entities.emplace<Rock::SphereComponent>(sphere, 0.5f);
    }

    {
        auto& transformComp = m_entities.get<Rock::TransformComponent>(obb2);
        transformComp.m_translation = glm::vec3(1.f, 1.f, 1.f);
        transformComp.recalculate();
        ASSERT_TRUE(Rock::obbIntersectingOBB(m_entities, obb1, obb2));
        transformComp.m_translation = glm::vec3(1.1f, 1.1f, 1.1f);
        transformComp.recalculate();
        ASSERT_FALSE(Rock::obbIntersectingOBB(m_entities, obb1, obb2));
    }

    {
        auto& transformComp = m_entities.get<Rock::TransformComponent>(sphere2);
        transformComp.m_translation = glm::vec3(0.57f, 0.57f, 0.57f);
        transformComp.recalculate();
        ASSERT_TRUE(Rock::sphereIntersectingSphere(m_entities, sphere1, sphere2));
        transformComp.m_translation = glm::vec3(0.58f, 0.58f, 0.58f);
        transformComp.recalculate();
        ASSERT_FALSE(Rock::sphereIntersectingSphere(m_entities, sphere1, sphere2));
    }

    {
        auto& transformComp = m_entities.get<Rock::TransformComponent>(sphere1);
        transformComp.m_translation = glm::vec3(0.78f, 0.78f, 0.78f);
        transformComp.recalculate();
        ASSERT_TRUE(Rock::obbIntersectingSphere(m_entities, obb1, sphere1));
        transformComp.m_translation = glm::vec3(0.79f, 0.79f, 0.79f);
        transformComp.recalculate();
        ASSERT_FALSE(Rock::obbIntersectingSphere(m_entities, obb1, sphere1));
    }
}

TEST(PhysicsEngine, TestRigidbodyComponent)
{
    auto& rigidbodyComp = m_entities.emplace<Rock::RigidbodyComponent>(gameObject, 100.f);
    
    auto& transformComp = m_entities.get<Rock::TransformComponent>(gameObject);
    ASSERT_EQ(transformComp.m_translation, glm::vec3(100.f, 100.f, 0.f));

    rigidbodyComp.m_grounded = false;
    for (int i = 0; i < 17; i++)
    {
        rigidbodyComp.update(1.f);
        transformComp.m_translation += rigidbodyComp.m_velocity;
        transformComp.recalculate();
        ASSERT_GT(transformComp.m_translation.y, 0.f);
    }

    rigidbodyComp.update(1.f);
    transformComp.m_translation += rigidbodyComp.m_velocity;
    transformComp.recalculate();
    ASSERT_LT(transformComp.m_translation.y, 0.f);
}

TEST(WindowTests, CreateWindow)
{
	ASSERT_TRUE(glfwInit());
	ASSERT_TRUE(glfwVulkanSupported);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Test Window", nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
}

TEST(DeviceTests, CheckValidationLayerSupport)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        ASSERT_TRUE(layerFound);
    }
}

TEST(DeviceTests, CheckInstanceExtensionSupport)
{
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(instanceExtensions.begin(), instanceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    ASSERT_TRUE(requiredExtensions.empty());
}

TEST(DeviceTests, CreateInstance)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    for (const char* extension : instanceExtensions)
    {
        extensions.push_back(extension);
    }

    ci.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    ci.enabledLayerCount = 0;
    ci.pNext = nullptr;
    ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();

    ASSERT_EQ(vkCreateInstance(&ci, nullptr, &m_instance), VK_SUCCESS);
}

TEST(DeviceTests, CreateSurface)
{
    ASSERT_EQ(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface), VK_SUCCESS);
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
            indices.graphicsFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

        if (presentSupport)
            indices.presentFamily = i;

        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && swapChainAdequate; // swapChainAdequate is false if extensionsSupported is false
}

TEST(DeviceTests, PickPhysicalDevice)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }

    ASSERT_NE(m_physicalDevice, VK_NULL_HANDLE);
}

TEST(DeviceTests, CreateLogicalDevice)
{
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    ci.pQueueCreateInfos = queueCreateInfos.data();
    ci.pEnabledFeatures = &deviceFeatures;
    ci.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    ci.ppEnabledExtensionNames = deviceExtensions.data();
    ci.enabledLayerCount = 0;

    if (vkCreateDevice(m_physicalDevice, &ci, nullptr, &m_device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device.");

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_queue);
}

TEST(DeviceTests, CreateCommandPool)
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    ASSERT_EQ(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool), VK_SUCCESS);
}

VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    return availableFormats[0];
}

VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            return availablePresentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    VkExtent2D actualExtent = { static_cast<uint32_t>(WIDTH), static_cast<uint32_t>(HEIGHT) };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &properties);
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
            return format;
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
            return format;
    }
}

VkFormat findDepthFormat()
{
    return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

TEST(SwapchainTests, CreateSwapchain)
{
    SwapChainSupportDetails swapchainSupport = querySwapChainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = choosePresentMode(swapchainSupport.presentModes);
    VkExtent2D extent = chooseExtent(swapchainSupport.capabilities);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
        imageCount = swapchainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = m_surface;
    ci.minImageCount = imageCount;
    ci.imageFormat = surfaceFormat.format;
    ci.imageColorSpace = surfaceFormat.colorSpace;
    ci.imageExtent = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // VK_IMAGE_USAGE_TRANSFER_DST_BIT to render images to a separate image first; e.g. post-processing
    ci.oldSwapchain = VK_NULL_HANDLE;

    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // images can be used across multiple queue families without explicit ownership transfers
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // an image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family
        ci.queueFamilyIndexCount = 0; // optional
        ci.pQueueFamilyIndices = nullptr; // optional
    }

    ci.preTransform = swapchainSupport.capabilities.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = presentMode;
    ci.clipped = VK_TRUE;

    ASSERT_EQ(vkCreateSwapchainKHR(m_device, &ci, nullptr, &m_swapchain), VK_SUCCESS);

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;
    m_swapchainDepthFormat = findDepthFormat();
}

TEST(SwapchainTests, CreateImageViews)
{
    m_swapchainImageViews.resize(m_swapchainImages.size());
    //m_swapchainImageMemory.resize(m_swapchainImages.size());

    for (size_t i = 0; i < m_swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.image = m_swapchainImages[i];
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ci.format = m_swapchainImageFormat;
        ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ci.subresourceRange.baseMipLevel = 0;
        ci.subresourceRange.levelCount = 1;
        ci.subresourceRange.baseArrayLayer = 0;
        ci.subresourceRange.layerCount = 1;

        VkImageView imageView;
        ASSERT_EQ(vkCreateImageView(m_device, &ci, nullptr, &imageView), VK_SUCCESS);

        m_swapchainImageViews[i] = imageView;
    }
}

TEST(SwapchainTests, CreateRenderPass)
{
    VkAttachmentDescription colourAttachment{};
    colourAttachment.format = m_swapchainImageFormat;
    colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colourAttachmentRef{};
    colourAttachmentRef.attachment = 0;
    colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkAttachmentDescription> attachments = { colourAttachment };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    ASSERT_EQ(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass), VK_SUCCESS);
}

TEST(SwapchainTests, CreateFramebuffers)
{
    m_framebuffers.resize(m_swapchainImageViews.size());

    for (size_t i = 0; i < m_swapchainImageViews.size(); i++)
    {
        std::vector<VkImageView> attachments = { m_swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;

        ASSERT_EQ(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffers[i]), VK_SUCCESS);
    }
}

TEST(SwapchainTests, CreateSyncObjects)
{
    m_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        ASSERT_EQ(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_semaphores[i]), VK_SUCCESS);
        ASSERT_EQ(vkCreateFence(m_device, &fenceInfo, nullptr, &m_fences[i]), VK_SUCCESS);
    }
}
