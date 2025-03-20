#pragma once
#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>

struct Vertex {
    float x;
    float y;
    float z;
    float u;
    float v;
};

struct VmaBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct VmaImage {
    VkImage image;
    VmaAllocation allocation;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};