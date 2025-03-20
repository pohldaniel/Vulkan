#pragma once
#include <vector>
#include "entity.h"

class Swapchain;
class VkContext;

class SwapchainElement
{
public:
    SwapchainElement(Swapchain* swapchain, VkImage image);
    SwapchainElement(const SwapchainElement& rhs) = delete;
    SwapchainElement(SwapchainElement&& rhs) = delete;
    ~SwapchainElement();

    SwapchainElement& operator=(const SwapchainElement& rhs) = delete;
    SwapchainElement& operator=(SwapchainElement&& rhs) = delete;

    void draw(const UniformBufferObject& ubo);

    VkContext* ctx;
    Swapchain* swapchain;

    VkImage image;
    VkImageView imageView;
    VkFramebuffer framebuffer;
    VkCommandBuffer commandBuffer;
    VkSemaphore startSemaphore;
    VkSemaphore endSemaphore;
    VkFence fence;
    VkFence lastFence = nullptr;

    VkDescriptorSet descriptorSet;
    int nextUniformIndex = 0;

    std::vector<Entity*> entities;

private:
    void prepareTexture();
    void imageToAttachmentLayout();
    void imageToPresentLayout();
};
