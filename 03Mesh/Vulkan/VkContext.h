#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "Data.h"

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
    void createUniformBuffers(const VkDevice& vkDevice);
    void createBuffer(const VkDevice& vkDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void updateUniformBuffer(const UniformBufferObject& ubo);

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

    std::vector<VkBuffer> vkBuffers;
    std::vector<VkDeviceMemory> vkDeviceMemory;
    std::vector<void*> vkBuffersMapped;

    UniformBufferObject ubo;

    Swapchain* swapchain;
    bool textureReady = false;
};

extern "C" {
    void vkInit(VkContext& vkContext, void* window);
    void vkDraw(VkContext& vkContext);
}