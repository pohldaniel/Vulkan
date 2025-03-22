#include "image.h"

VkImageView create_image_view(VkDevice logicalDevice, VkImage image, VkFormat format) {
    
    /*
    * ImageViewCreateInfo( VULKAN_HPP_NAMESPACE::ImageViewCreateFlags flags_ = {},
        VULKAN_HPP_NAMESPACE::Image                image_ = {},
        VULKAN_HPP_NAMESPACE::ImageViewType    viewType_  = VULKAN_HPP_NAMESPACE::ImageViewType::e1D,
        VULKAN_HPP_NAMESPACE::Format           format_    = VULKAN_HPP_NAMESPACE::Format::eUndefined,
        VULKAN_HPP_NAMESPACE::ComponentMapping components_            = {},
        VULKAN_HPP_NAMESPACE::ImageSubresourceRange subresourceRange_ = {} ) VULKAN_HPP_NOEXCEPT
        : flags( flags_ )
        , image( image_ )
        , viewType( viewType_ )
        , format( format_ )
        , components( components_ )
        , subresourceRange( subresourceRange_ )
    */

	VkImageViewCreateInfo createInfo = {};
    createInfo.image = image;
    createInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

	VkImageView view;
	vkCreateImageView(logicalDevice, &createInfo, 0, &view);

    return view;
}

void transition_image_layout(VkCommandBuffer commandBuffer, VkImage image,
	VkImageLayout oldLayout, VkImageLayout newLayout,
	VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
	VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {

	VkImageSubresourceRange access;
	access.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	access.baseMipLevel = 0;
	access.levelCount = 1;
	access.baseArrayLayer = 0;
	access.layerCount = 1;
	
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = access;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;


	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, 0, 0, 0, 1, &barrier);

}