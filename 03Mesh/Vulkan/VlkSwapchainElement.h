#pragma once
#include <vector>
#include "Data.h"

struct VlkContext;
class VlkSwapchain;
class VlkTexture;
class VlkMesh;

class VlkSwapchainElement
{
public:
    VlkSwapchainElement(VlkSwapchain* swapchain, VkImage image, VkImage depthImage);
    VlkSwapchainElement(const VlkSwapchainElement& rhs) = delete;
    VlkSwapchainElement(VlkSwapchainElement&& rhs) = delete;
    ~VlkSwapchainElement();

    VlkSwapchainElement& operator=(const VlkSwapchainElement& rhs) = delete;
    VlkSwapchainElement& operator=(VlkSwapchainElement&& rhs) = delete;

    void draw(const UniformBufferObject& ubo, const std::list<VlkMesh*>& meshes, std::vector<VlkTexture>& textures);

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




   

    int descriptorIndex;

    void draw(const VkCommandBuffer& vkCommandbuffer, const UniformBufferObject& ubo, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount, VlkTexture* texture);

};
