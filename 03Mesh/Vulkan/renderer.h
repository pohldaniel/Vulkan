#pragma once
#define GLFW_INCLUDE_VULKAN


#include <deque>
#include <functional>
#include <vector>

#include "instance.h"
#include "frame.h"
#include "swapchain.h"
#include "entity.h"

struct Vertex{
    float x;
    float y;
    float z;
    float u;
    float v;
};


class Swapchain2;


class Engine {

public:

    Engine(void* window, int width, int height);
    ~Engine();
    void draw();
    void createMesh();
    void createTexture();
    void createAllocator();

    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createPushConstantRange();
    void createPipelineLayout();
    void createSampler();
    void createShaders();
    void createTextureView();
    void resize();

    VmaAllocator memoryAllocator;
    VmaBuffer mesh;
    VmaImage texture;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPushConstantRange pushConstantRange;
    VkPipelineLayout pipelineLayout;
    VkSampler sampler;
    VkImageView textureView;


    std::deque<std::function<void(VkInstance)>> instanceDeletionQueue;
    std::deque<std::function<void(VkDevice)>> deviceDeletionQueue;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    Swapchain swapchain;
    Swapchain2* swapchain2;
    std::vector<Frame> frames;
    std::vector<VkShaderEXT> _shaders;
    VkShaderEXT shaders[2];
    std::vector<VkShaderEXT> meshShaders;
    VkCommandPool commandPool;
    VkRenderingInfoKHR renderingInfo;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t imageIndex = 0;
    bool textureReady = false;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    std::vector <VkPipelineStageFlags> waitStages;
};