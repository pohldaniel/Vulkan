#pragma once
#include <vector>
#include <Vulkan/vulkan.h>
#include "swapchain_element.h"

struct VlkContext;

class Swapchain{

public:
    Swapchain(VlkContext* ctx, unsigned width, unsigned height, const VkPresentModeKHR vkPresentModeKHR, VkSwapchainKHR vkOldSwapchainKHR = NULL);
    Swapchain(const Swapchain& rhs) = delete;
    Swapchain(Swapchain&& rhs) = delete;
    ~Swapchain();

    Swapchain& operator=(const Swapchain& rhs) = delete;
    Swapchain& operator=(Swapchain&& rhs) = delete;

    bool draw(const UniformBufferObject& ubo, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount);

    VlkContext* ctx;

    VkSwapchainKHR swapchain;
    VkFormat format;
    unsigned width;
    unsigned height;
    uint32_t imageCount;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImagesMemory;
    std::vector<VkImage> images;

private:
    uint32_t currentFrame;
    uint32_t imageIndex;
    std::vector<SwapchainElement*> elements;
};
