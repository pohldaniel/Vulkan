#pragma once
#include <vulkan/vulkan.h>
#include <deque>
#include <functional>
#include <vector>

bool supported_by_instance(const char** extensionNames, int extensionCount, const char** layerNames, int layerCount);
VkInstance make_instance(const char* applicationName, std::deque<std::function<void(VkInstance)>>& deletionQueue);