#pragma once
#include <vector>
#include "entity.h"

class Swapchain;
struct VlkContext;

class SwapchainElement
{
public:
    SwapchainElement(Swapchain* swapchain, VkImage image, VkImage depthImage);
    SwapchainElement(const SwapchainElement& rhs) = delete;
    SwapchainElement(SwapchainElement&& rhs) = delete;
    ~SwapchainElement();

    SwapchainElement& operator=(const SwapchainElement& rhs) = delete;
    SwapchainElement& operator=(SwapchainElement&& rhs) = delete;

    void draw(const UniformBufferObject& ubo, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount);

    VlkContext* ctx;
    Swapchain* swapchain;

    VkCommandBuffer commandBuffer;

    VkImage image;
    VkImageView imageView;

    VkImage depthImage;
    VkImageView depthImageView;
    //VkDeviceMemory depthImageMemory;

    VkSemaphore startSemaphore;
    VkSemaphore endSemaphore;
    VkFence fence;
    VkFence lastFence = nullptr;

    VkDescriptorSet descriptorSet;
    int nextUniformIndex = 0;

    VkRenderingAttachmentInfo colorAttachment;
    VkRenderingAttachmentInfo depthStencilAttachment;
    VkRenderingInfo renderingInfo;
    VkViewport viewport;
    VkRect2D scissor;

    std::vector<Entity*> entities;

};
