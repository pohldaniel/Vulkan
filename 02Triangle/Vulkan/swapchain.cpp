#include <iostream>
#include "swapchain.h"

void Swapchain::build(
    VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface, uint32_t width, uint32_t height,
    std::deque<std::function<void(VkDevice)>>& deviceDeletionQueue) {

    SurfaceDetails support = query_surface_support(physicalDevice, surface);

    format = choose_surface_format(support.formats);

    VkPresentModeKHR presentMode = choose_present_mode(support.presentModes);

    extent = choose_extent(width, height, support.capabilities);

    imageCount = min(support.capabilities.maxImageCount,support.capabilities.minImageCount + 1);

    /*
    * VULKAN_HPP_CONSTEXPR SwapchainCreateInfoKHR(
    VULKAN_HPP_NAMESPACE::SwapchainCreateFlagsKHR flags_         = {},
    VULKAN_HPP_NAMESPACE::SurfaceKHR              surface_       = {},
    uint32_t                                      minImageCount_ = {},
    VULKAN_HPP_NAMESPACE::Format                  imageFormat_   = VULKAN_HPP_NAMESPACE::Format::eUndefined,
    VULKAN_HPP_NAMESPACE::ColorSpaceKHR   imageColorSpace_  = VULKAN_HPP_NAMESPACE::ColorSpaceKHR::eSrgbNonlinear,
    VULKAN_HPP_NAMESPACE::Extent2D        imageExtent_      = {},
    uint32_t                              imageArrayLayers_ = {},
    VULKAN_HPP_NAMESPACE::ImageUsageFlags imageUsage_       = {},
    VULKAN_HPP_NAMESPACE::SharingMode     imageSharingMode_ = VULKAN_HPP_NAMESPACE::SharingMode::eExclusive,
    uint32_t                              queueFamilyIndexCount_ = {},
    const uint32_t *                      pQueueFamilyIndices_   = {},
    VULKAN_HPP_NAMESPACE::SurfaceTransformFlagBitsKHR preTransform_ =
    VULKAN_HPP_NAMESPACE::SurfaceTransformFlagBitsKHR::eIdentity,
    VULKAN_HPP_NAMESPACE::CompositeAlphaFlagBitsKHR compositeAlpha_ =
    VULKAN_HPP_NAMESPACE::CompositeAlphaFlagBitsKHR::eOpaque,
    VULKAN_HPP_NAMESPACE::PresentModeKHR presentMode_  = VULKAN_HPP_NAMESPACE::PresentModeKHR::eImmediate,
    VULKAN_HPP_NAMESPACE::Bool32         clipped_      = {},
    VULKAN_HPP_NAMESPACE::SwapchainKHR   oldSwapchain_ = {} ) VULKAN_HPP_NOEXCEPT
    */
    
    /*VkSwapchainCreateInfoKHR createInfo =
    vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR(), 
        surface, imageCount, format.format, format.colorSpace,
        extent, 1, vk::ImageUsageFlagBits::eColorAttachment);

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = vk::SwapchainKHR(nullptr);*/

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    //auto result = logicalDevice.createSwapchainKHR(createInfo);

    auto result = vkCreateSwapchainKHR(logicalDevice, &createInfo, 0, &chain);
    if (result  == VkResult::VK_SUCCESS) {    
        deviceDeletionQueue.push_back([this](VkDevice device){
            std::cout << "Destroyed swapchain" << std::endl;
            vkDestroySwapchainKHR(device, chain, NULL);
        });
    }else {
        std::cout << "failed to create swap chain!" << std::endl;
    }
}

SurfaceDetails Swapchain::query_surface_support(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface) {
	
    SurfaceDetails support;
    VkSurfaceCapabilitiesKHR surfaceCaps = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &support.capabilities);

    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, NULL);
    VkSurfaceFormatKHR* surfaceFormat = new VkSurfaceFormatKHR[count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, surfaceFormat);
    support.formats = std::vector<VkSurfaceFormatKHR>(surfaceFormat, surfaceFormat + count);

    uint32_t count2 = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count2, NULL);
    VkPresentModeKHR* presentModes = new VkPresentModeKHR[count2];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count2, presentModes);
    support.presentModes = std::vector<VkPresentModeKHR>(presentModes, presentModes + count2);

	return support;
}

VkSurfaceFormatKHR Swapchain::choose_surface_format(
    std::vector<VkSurfaceFormatKHR> formats) {

    for (VkSurfaceFormatKHR format : formats) {
        if (format.format == VkFormat::VK_FORMAT_B8G8R8A8_UNORM
            && format.colorSpace == VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formats[0];
}

VkPresentModeKHR Swapchain::choose_present_mode(
    std::vector<VkPresentModeKHR> presentModes) {
    
    for (VkPresentModeKHR presentMode : presentModes) {
        if (presentMode == VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::choose_extent(uint32_t width, uint32_t height, VkSurfaceCapabilitiesKHR capabilities) {

    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        VkExtent2D extent = { width, height };

        extent.width = min(
            capabilities.maxImageExtent.width, 
            max(capabilities.minImageExtent.width, extent.width)
        );

        extent.height = min(
            capabilities.maxImageExtent.height,
            max(capabilities.minImageExtent.height, extent.height)
        );

        return extent;
    }
}
