#pragma once
#include <vector>
#include "entity.h"
#include "VlkMesh.h"

class VlkSwapchain;
struct VlkContext;

class VlkSwapchainElement
{
public:
    VlkSwapchainElement(VlkSwapchain* swapchain, VkImage image, VkImage depthImage);
    VlkSwapchainElement(const VlkSwapchainElement& rhs) = delete;
    VlkSwapchainElement(VlkSwapchainElement&& rhs) = delete;
    ~VlkSwapchainElement();

    VlkSwapchainElement& operator=(const VlkSwapchainElement& rhs) = delete;
    VlkSwapchainElement& operator=(VlkSwapchainElement&& rhs) = delete;

    void draw(const UniformBufferObject& ubo, const std::list<VlkMesh*>& meshes);

    VlkContext* ctx;
    VlkSwapchain* swapchain;

    VkCommandBuffer commandBuffer;

    VkImage image;
    VkImageView imageView;

    VkImage depthImage;
    VkImageView depthImageView;

    VkSemaphore startSemaphore;
    VkSemaphore endSemaphore;
    VkFence fence;
    VkFence lastFence = nullptr;

   
    int nextUniformIndex = 0;

    VkRenderingAttachmentInfo colorAttachment;
    VkRenderingAttachmentInfo depthStencilAttachment;
    VkRenderingInfo renderingInfo;
    VkViewport viewport;
    VkRect2D scissor;


    std::vector<Entity*> entities;
};
