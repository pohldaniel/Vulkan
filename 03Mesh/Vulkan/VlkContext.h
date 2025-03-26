#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>
#include "Data.h"

class VlkSwapchain;

struct VlkContext {

    bool createVkDevice(VlkContext& vlkcontext, void* window); 
    void createMesh();
    void createTexture();
    void createShaders(const VkDevice& vkDevice);
    void createDescriptorPool(const VkDevice& vkDevice);
    void createDescriptorSetLayout(const VkDevice& vkDevice);
    void createPushConstantRange(const VkDevice& vkDevice);
    void createPipelineLayout(const VkDevice& vkDevice);

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
    std::vector<VkShaderEXT> shader;
    VmaImage vmaImage;
    VmaBuffer vmaBuffer;
    VmaAllocator memoryAllocator;
    VkQueue vkQueue;
    VkCommandPool vkCommandPool;
    VkCommandBuffer vkCommandBuffer;
    VkFormat vkDepthFormat;
    UniformBufferObject ubo;
    VlkSwapchain* swapchain;
    VlkSwapchain* newSwapchain;
    VkDescriptorSet descriptorSet;
    bool textureReady = false;

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
    void vlkCopyBufferToImage(const VkBuffer& srcBuffer, const VkImage& dstImage, uint32_t width, uint32_t height);

    void vlkCreateCommandPool(VkCommandPool& vkCommandPool);
    void vlkCreateCommandBuffer(VkCommandBuffer& vkCommandBuffer);

    void vlkTransitionImageLayout(const VkCommandBuffer& commandBuffer, const VkImage& vkImage, VkImageAspectFlags vkImageAspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);  
    void vlkCreateSemaphore(VkSemaphore& vkSemaphore);
    void vlkCreateFence(VkFence& vkFence);
    void vlkBeginCommandBuffer(const VkCommandBuffer& vkCommandBuffer, VkCommandBufferUsageFlags vkCommandBufferUsageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    void vlkEndCommandBuffer(const VkCommandBuffer& vkCommandBuffer, bool submit = false);
    void vlkQueueSubmit(const VkCommandBuffer& vkCommandBuffer);

    void vlkCreateSwapChain(VkSwapchainKHR& vkSwapchainKHR, VkFormat& vkFormat, uint32_t width, uint32_t height, const VkPresentModeKHR vkPresentModeKHR = VK_PRESENT_MODE_FIFO_KHR, VkSwapchainKHR vkOldSwapchainKHR = NULL);

    void vlkReadFile(const char* filename, std::vector<char>& data);
    void vlkCompileSahder(const char* path, shaderc_shader_kind kind, std::vector<uint32_t>& shaderCode);
    std::vector<VkShaderEXT>& vlkCreateShader(const VkDescriptorSetLayout& vkDescriptorSetLayout, const VkPushConstantRange& vkPushConstantRange, const std::vector<uint32_t>& vertexCode, const std::vector<uint32_t>& fragmentCode, std::vector<VkShaderEXT>& shader);

    void vlkCreateSampler(VkSampler& vkSampler);
    void vlkCreateAllocator(VmaAllocator& vmaAllocator);

    uint32_t vlkFindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void vlkReadImageFile(VkImage& vkImage, VkDeviceMemory& vkDeviceMemory, const char* fileName, int& width, int& height, const bool flipVertical = false);
    void vlkBindImageViewToDescriptorSet(const VkImageView& vkImageView, const VkDescriptorSet& vkDescriptorSet, uint32_t dstBinding);
   
};