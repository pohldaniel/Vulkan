#pragma once


#include "Data.h"

class VlkSwapchainElement;
struct VlkContext;

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
    Entity(VlkSwapchainElement* element, float x, float y);
    Entity(const Entity& rhs) = delete;
    Entity(Entity&& rhs) = delete;
    ~Entity();
    void createMVP();

    Entity& operator=(const Entity& rhs) = delete;
    Entity& operator=(Entity&& rhs) = delete;

    void draw(const UniformBufferObject& ubo, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount);

    VlkContext* ctx;
    VlkSwapchainElement* element;

    VmaBuffer uniform;
    EntityUniform* uniformMapping;

    VmaBuffer uniformMVP;
    UniformBufferObject* uniformMappingMVP;

private:
    int descriptorIndex;
};
