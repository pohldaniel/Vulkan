#pragma once
#include <vulkan/vulkan.h>
#include <deque>
#include <functional>
#include <iostream>
#include <vector>

bool supports(const VkPhysicalDevice& device, const char** ppRequestedExtensions, const uint32_t requestedExtensionCount);
bool is_suitable(const VkPhysicalDevice& device);
VkPhysicalDevice choose_physical_device(const VkInstance& instance);
uint32_t find_queue_family_index(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkQueueFlags queueType);
VkDevice create_logical_device(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::deque<std::function<void(VkDevice)>>& deletionQueue);