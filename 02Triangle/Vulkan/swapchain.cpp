#include <iostream>
#include "swapchain.h"

void Swapchain::build(
    vk::Device logicalDevice, vk::PhysicalDevice physicalDevice, 
    vk::SurfaceKHR surface, uint32_t width, uint32_t height,
    std::deque<std::function<void(vk::Device)>>& deviceDeletionQueue) {

    SurfaceDetails support = query_surface_support(
        physicalDevice, surface);

    format = choose_surface_format(support.formats);

    vk::PresentModeKHR presentMode = choose_present_mode(
        support.presentModes);

    extent = choose_extent(width, height, support.capabilities);

    imageCount = min(
        support.capabilities.maxImageCount,
        support.capabilities.minImageCount + 1
    );

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
    vk::SwapchainCreateInfoKHR createInfo = 
    vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR(), 
        surface, imageCount, format.format, format.colorSpace,
        extent, 1, vk::ImageUsageFlagBits::eColorAttachment);

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

    auto result = logicalDevice.createSwapchainKHR(createInfo);
    if (result.result == vk::Result::eSuccess) {
        chain = result.value;

        deviceDeletionQueue.push_back([this](vk::Device device){
            std::cout << "Destroyed swapchain" << std::endl;
            device.destroySwapchainKHR(chain);
        });
    }
    else {
        std::cout << "failed to create swap chain!" << std::endl;
    }
}

SurfaceDetails Swapchain::query_surface_support(
        vk::PhysicalDevice physicalDevice, 
        vk::SurfaceKHR surface) {
	
    SurfaceDetails support;
    support.capabilities = physicalDevice
        .getSurfaceCapabilitiesKHR(surface).value;

	
    support.formats = physicalDevice
        .getSurfaceFormatsKHR(surface).value;


	support.presentModes = physicalDevice
        .getSurfacePresentModesKHR(surface).value;
    std::cout << "Supported Present Modes:" << std::endl;

	
	return support;
}

vk::SurfaceFormatKHR Swapchain::choose_surface_format(
    std::vector<vk::SurfaceFormatKHR> formats) {

    for (vk::SurfaceFormatKHR format : formats) {
        if (format.format == vk::Format::eB8G8R8A8Unorm
            && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }

    return formats[0];
}

vk::PresentModeKHR Swapchain::choose_present_mode(
    std::vector<vk::PresentModeKHR> presentModes) {
    
    for (vk::PresentModeKHR presentMode : presentModes) {
        if (presentMode == vk::PresentModeKHR::eMailbox) {
            return presentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::choose_extent(
    uint32_t width, uint32_t height, 
    vk::SurfaceCapabilitiesKHR capabilities) {

    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        vk::Extent2D extent = { width, height };

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
