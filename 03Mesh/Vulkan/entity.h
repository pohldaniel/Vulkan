#pragma once

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

class SwapchainElement;
class VkContext;

#pragma pack(1)
struct EntityUniform{
    float x;
    float y;
    float rotation;
    float scale;
    float aspect;
};
#pragma pack()

class Entity
{
public:
    Entity(SwapchainElement* element, float x, float y);
    Entity(const Entity& rhs) = delete;
    Entity(Entity&& rhs) = delete;
    ~Entity();

    Entity& operator=(const Entity& rhs) = delete;
    Entity& operator=(Entity&& rhs) = delete;

    void draw();

    VkContext* ctx;
    SwapchainElement* element;

    VmaBuffer uniform;
    EntityUniform* uniformMapping;

private:
    int descriptorIndex;
};
