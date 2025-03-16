#pragma once
#define GLFW_INCLUDE_VULKAN

#include <deque>
#include <functional>
#include "instance.h"
#include "frame.h"
#include "swapchain.h"
#include <vector>

class Engine {

public:


    Engine(void* window, int width, int height);
    ~Engine();
    void draw();

private:

    std::deque<std::function<void(VkInstance)>> instanceDeletionQueue;
    std::deque<std::function<void(VkDevice)>> deviceDeletionQueue;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    Swapchain swapchain;
    std::vector<Frame> frames;
    std::vector<VkShaderEXT> shaders;
    VkCommandPool commandPool;
    VkRenderingInfoKHR renderingInfo;
    uint32_t graphicsQueueFamilyIndex;
};