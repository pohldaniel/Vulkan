#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "Data.h"

class VlkSwapchain;

struct VlkContext {

    bool createVkDevice(VlkContext& vlkcontext, void* window);
    void createCommandPool();
    void createCommandBuffer();
    void copyBuffer(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, uint32_t size);
    void createBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void mapBuffer(const VkDeviceMemory& bufferMemory, const void* data, uint32_t size);

    void createMesh();
    void createTexture();
    void createAllocator();
   
 
    void createShaders(const VkDevice& vkDevice);
    void createDescriptorPool(const VkDevice& vkDevice);
    void createDescriptorSetLayout(const VkDevice& vkDevice);
    void createPushConstantRange(const VkDevice& vkDevice);
    void createPipelineLayout(const VkDevice& vkDevice);
    void createSampler(const VkDevice& vkDevice);
  
    void createTextureView(const VkDevice& vkDevice);
    void resize();
    void createUniformBuffers(const VkDevice& vkDevice);
    void createBuffer(const VkDevice& vkDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void updateUniformBuffer(const UniformBufferObject& ubo);
    uint32_t GetMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    VkExtent2D screenSize;
    VkInstance vkInstance;
    VkDevice vkDevice;
    VkPhysicalDevice vkPhysicalDevice; 
    VkDebugUtilsMessengerEXT vkDebugMessenger;

    VkSurfaceKHR vkSurfaceKHR;
    uint32_t queueFamilyIndex;

    const int maxDescriptorSets = 10;
    const int maxDescriptorCount = 65536;
    const int pushConstantRangeSize = 128;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPushConstantRange pushConstantRange;
    VkPipelineLayout pipelineLayout;
    VkSampler sampler;
    VkImageView textureView;
    VkShaderEXT shaders[2];
    VmaImage vmaImage;
    VmaBuffer vmaBuffer;
    VmaAllocator memoryAllocator;
    VkQueue vkQueue;
    VkCommandPool vkCommandPool;
    VkCommandBuffer vkCommandBuffer;
    VkFormat vkDepthFormat;

    std::vector<VkBuffer> vkBuffers;
    std::vector<VkDeviceMemory> vkDeviceMemory;
    std::vector<void*> vkBuffersMapped;

    UniformBufferObject ubo;

    VlkSwapchain* swapchain;
    VlkSwapchain* newSwapchain;

    bool textureReady = false;
    bool depthReady = false;

    VkPolygonMode vkPolygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    VkPresentModeKHR vkPresentModeKHR = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
};

extern VlkContext vlkContext;

extern "C" {
    void vlkInit(void* window);
    void vlkResize();
    void vlkToggleVerticalSync();
    void vlkToggleWireframe();
    void vlkDraw(const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount);

    void vlkMapBuffer(const VkDeviceMemory& vkDeviceMemory, const void* data, uint32_t size);
    void vlkCreateBuffer(VkBuffer& vkBuffer, VkDeviceMemory& vkDeviceMemory, uint32_t size, VkBufferUsageFlags vkBufferUsageFlags, VkMemoryPropertyFlags vkMemoryPropertyFlags);
    void vlkCopyBuffer(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, uint32_t size);
   

    void vlkCreateImage(VkImage& vkImage, VkDeviceMemory& vkDeviceMemory, uint32_t width, uint32_t height, VkFormat vkFormat, VkImageTiling vkImageTiling, VkImageUsageFlags vkImageUsageFlags, VkMemoryPropertyFlags vkMemoryPropertyFlags);
    void vlkCreateImageView(VkImageView& vkImageView, const VkImage& vkImage, VkFormat vkFormat, VkImageAspectFlags vkImageAspectFlags, VkComponentMapping vkComponentMapping = {});  
    void vlkDestroyImage(const VkImage& vkImage, const VkDeviceMemory& vkDeviceMemory = NULL);

    void vlkCreateCommandBuffer(VkCommandBuffer& vkCommandBuffer);
    void vlkTransitionImageLayout(const VkCommandBuffer& commandBuffer, const VkImage& vkImage, VkImageAspectFlags vkImageAspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);  
    void vlkCreateSemaphore(VkSemaphore& vkSemaphore);
    void vlkCreateFence(VkFence& vkFence);
    void vlkBeginCommandBuffer(const VkCommandBuffer& vkCommandBuffer, VkCommandBufferUsageFlags vkCommandBufferUsageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    void vlkEndCommandBuffer(const VkCommandBuffer& vkCommandBuffer);
    void vlkQueueSubmit(const VkCommandBuffer& vkCommandBuffer);

    void vlkCreateSwapChain(VkSwapchainKHR& vkSwapchainKHR, VkFormat& vkFormat, uint32_t width, uint32_t height, const VkPresentModeKHR vkPresentModeKHR = VK_PRESENT_MODE_FIFO_KHR, VkSwapchainKHR vkOldSwapchainKHR = NULL);
};