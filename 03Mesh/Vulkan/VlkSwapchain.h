#pragma once
#include <vector>
#include <list>
#include <Vulkan/vulkan.h>
#include "VlkSwapchainElement.h"


struct VlkContext;
class VlkTexture;

class VlkSwapchain{

public:
    VlkSwapchain(VlkContext* ctx, unsigned width, unsigned height, const VkPresentModeKHR vkPresentModeKHR, VkSwapchainKHR vkOldSwapchainKHR = NULL);
    VlkSwapchain(const VlkSwapchain& rhs) = delete;
    VlkSwapchain(VlkSwapchain&& rhs) = delete;
    ~VlkSwapchain();

    VlkSwapchain& operator=(const VlkSwapchain& rhs) = delete;
    VlkSwapchain& operator=(VlkSwapchain&& rhs) = delete;

    bool draw(const std::list<VlkMesh>& meshes);

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
    std::vector<VlkSwapchainElement*> elements;
};
