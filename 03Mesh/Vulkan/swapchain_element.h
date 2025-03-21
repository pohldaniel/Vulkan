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

    void draw(const UniformBufferObject& ubo, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount);

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

    VkImage depthImage;
    VkImageView depthImageView;
    VkDeviceMemory depthImageMemory;

    std::vector<Entity*> entities;

private:
    void prepareTexture();
    void imageToAttachmentLayout();
    void imageToPresentLayout();

    void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& vkImageView);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
};
