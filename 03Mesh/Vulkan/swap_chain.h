#pragma once
#include <vector>
#include <Vulkan/vulkan.h>
#include "swapchain_element.h"

class VkContext;

class Swapchain{

public:
    Swapchain(VkContext* ctx, unsigned width, unsigned height);
    Swapchain(const Swapchain& rhs) = delete;
    Swapchain(Swapchain&& rhs) = delete;
    ~Swapchain();

    Swapchain& operator=(const Swapchain& rhs) = delete;
    Swapchain& operator=(Swapchain&& rhs) = delete;

    bool draw();

    VkContext* ctx;

    VkSwapchainKHR swapchain;
    VkFormat format;
    unsigned width;
    unsigned height;

private:
    uint32_t currentFrame;
    uint32_t imageIndex;
    std::vector<SwapchainElement*> elements;
};
