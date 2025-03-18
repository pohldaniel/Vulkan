#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

struct VmaImage2 {
    VkImage image;
    VmaAllocation allocation;
};

struct VmaBuffer2 {
    VkBuffer buffer;
    VmaAllocation allocation;
};

class Swapchain;

struct VkContext {

    bool createVkDevice(VkContext& vkcontext, void* window);
    void createCommandPool();
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
    VmaImage2 vmaImage;
    VmaBuffer2 vmaBuffer;
    VmaAllocator memoryAllocator;
    VkQueue vkQueue;
    VkCommandPool vkCommandPool;

    Swapchain* swapchain;
    bool textureReady = false;
};

extern "C" {
    void vkInit(VkContext& vkContext, void* window);
    void vkDraw(VkContext& vkContext);
}