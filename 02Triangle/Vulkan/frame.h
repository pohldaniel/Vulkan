#pragma once

#include <vulkan/vulkan.h>
#include "image.h"
#include <deque>
#include <functional>
#include <vector>

class Frame {
public:
   
    Frame(VkImage image, VkDevice logicalDevice, VkFormat swapchainFormat, std::deque<std::function<void(VkDevice)>>& deletionQueue);
    void set_command_buffer(VkInstance instance, VkCommandBuffer newCommandBuffer, std::vector<VkShaderEXT>& shaders, VkExtent2D frameSize);

    VkImage image;
    VkImageView imageView;
    VkCommandBuffer commandBuffer;

private:

   
    void build_rendering_info(VkExtent2D frameSize);
    void build_color_attachment(); 
    void annoying_boilerplate_that_dynamic_rendering_was_meant_to_spare_us(VkInstance instance, VkExtent2D frameSize);

    VkRenderingInfoKHR renderingInfo = {};
    VkRenderingAttachmentInfoKHR colorAttachment = {};
};