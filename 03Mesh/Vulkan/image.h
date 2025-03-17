#pragma once

#include <vulkan/vulkan.h>


VkImageView create_image_view(VkDevice logicalDevice,
    VkImage image, VkFormat format);

void transition_image_layout(VkCommandBuffer commandBuffer, VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);