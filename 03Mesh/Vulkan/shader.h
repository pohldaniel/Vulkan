#pragma once
#include <vulkan/vulkan.h>
#include <deque>
#include <functional>
#include <vector>


std::vector<VkShaderEXT> make_shader_objects(VkInstance instance, VkDevice logicalDevice,const char* name, std::deque<std::function<void(VkDevice)>>& deviceDeletionQueue);

std::vector<char> read_file(const char* filename);