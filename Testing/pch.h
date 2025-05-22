//
// pch.h
//

#pragma once

// STL headers
#include <iostream>
#include <stdexcept>
#include <vector>
#include <optional>
#include <set>
#include <memory>
#include <algorithm>
#include <string>
#include <fstream>
#include <random>
#include <chrono>

// Vulkan header
#include <vulkan/vulkan.h>

// GLFW header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM headers
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

// stb image header
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// tiny obj loader header
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// imgui headers
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#define IMGUI_ENABLE_FREETYPE  // higher quality font rendering

#include "window/window.hpp"
#include "window/eventSystem.hpp"
#include "core/device.hpp"
#include "rendering/swapchain.hpp"
#include "rendering/pipeline.hpp"
#include "core/descriptors.hpp"
#include "rendering/renderer.hpp"
#include "core/application.hpp"
#include "examples/modelApp.hpp"
#include "examples/computeApp.hpp"
#include "examples/engineApp.hpp"

// physics engine
#include "math.hpp"

// used for google test
#include "gtest/gtest.h"
