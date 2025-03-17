#pragma once
#define GLFW_INCLUDE_VULKAN

#include <vma/vk_mem_alloc.h>
#include <deque>
#include <functional>
#include <vector>

#include "instance.h"
#include "frame.h"
#include "swapchain.h"


struct Vertex{
    float x;
    float y;
    float z;
    float u;
    float v;
};

struct VmaBuffer{
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct VmaImage{
    VkImage image;
    VmaAllocation allocation;
};

class Engine {

public:


    Engine(void* window, int width, int height);
    ~Engine();
    void draw();
    void createMesh();
    void createTexture();
    void createAllocator();

    VmaAllocator memoryAllocator;
    VmaBuffer mesh;
    VmaImage texture;

private:

    std::deque<std::function<void(VkInstance)>> instanceDeletionQueue;
    std::deque<std::function<void(VkDevice)>> deviceDeletionQueue;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    Swapchain swapchain;
    std::vector<Frame> frames;
    std::vector<VkShaderEXT> shaders;
    VkCommandPool commandPool;
    VkRenderingInfoKHR renderingInfo;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t imageIndex = 0;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    std::vector <VkPipelineStageFlags> waitStages;
};