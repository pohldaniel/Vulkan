#pragma once

#include <vulkan/vulkan.h>
#include <deque>
#include <functional>
#include <iostream>


VkCommandPool make_command_pool(VkDevice logicalDevice, uint32_t queueFamilyIndex, std::deque<std::function<void(VkDevice)>>& deletionQueue);

VkCommandBuffer allocate_command_buffer(VkDevice logicalDevice, VkCommandPool commandPool);