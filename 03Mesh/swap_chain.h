#pragma once
#include <vector>
#include <Vulkan/vulkan.h>
#include "swapchain_element.h"

class Engine;

class Swapchain2
{
public:
    Swapchain2(Engine* ctx, unsigned width, unsigned height);
    Swapchain2(const Swapchain2& rhs) = delete;
    Swapchain2(Swapchain2&& rhs) = delete;
    ~Swapchain2();

    Swapchain2& operator=(const Swapchain2& rhs) = delete;
    Swapchain2& operator=(Swapchain2&& rhs) = delete;

    bool draw();

    Engine* ctx;

    VkSwapchainKHR swapchain;
    VkFormat format;
    unsigned width;
    unsigned height;

private:
    uint32_t currentFrame;
    uint32_t imageIndex;
    std::vector<SwapchainElement*> elements;
};
