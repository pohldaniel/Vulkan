#define NOMINMAX
#include <algorithm>
#include <iostream>
#include <limits>
#include "swap_chain.h"
#include "Vulkan/VkContext.h"

Swapchain::Swapchain(VkContext* ctx, unsigned width, unsigned height)
    : ctx(ctx)
    , width(width)
    , height(height)
    , currentFrame(0)
    , imageIndex(0)
{
    VkResult result;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vkPhysicalDevice, ctx->vkSurfaceKHR, &capabilities);

    width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vkPhysicalDevice, ctx->vkSurfaceKHR, &formatCount, nullptr);

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vkPhysicalDevice, ctx->vkSurfaceKHR, &formatCount, formats.data());

    VkSurfaceFormatKHR chosenFormat = formats[0];

    for (const VkSurfaceFormatKHR& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM)
        {
            chosenFormat = format;
            break;
        }
    }

    format = chosenFormat.format;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = ctx->vkSurfaceKHR;
    createInfo.minImageCount = capabilities.minImageCount;
    createInfo.imageFormat = chosenFormat.format;
    createInfo.imageColorSpace = chosenFormat.colorSpace;
    createInfo.imageExtent = { width, height };
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = true;

    vkCreateSwapchainKHR(ctx->vkDevice, &createInfo, nullptr, &swapchain);

    uint32_t imageCount;
    vkGetSwapchainImagesKHR(ctx->vkDevice, swapchain, &imageCount, nullptr);

    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(ctx->vkDevice, swapchain, &imageCount, images.data());

    for (VkImage image : images)
    {
        elements.push_back(new SwapchainElement(this, image));
    }
}

Swapchain::~Swapchain()
{
    for (SwapchainElement* element : elements)
    {
        delete element;
    }
    elements.clear();
    vkDestroySwapchainKHR(ctx->vkDevice, swapchain, nullptr);
}

bool Swapchain::draw(const UniformBufferObject& ubo, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount)
{
    VkResult result;

    const SwapchainElement* currentElement = elements.at(currentFrame);

    vkWaitForFences(ctx->vkDevice, 1, &currentElement->fence, true, std::numeric_limits<uint64_t>::max());

    result = vkAcquireNextImageKHR(
        ctx->vkDevice,
        swapchain,
        std::numeric_limits<uint64_t>::max(),
        currentElement->startSemaphore,
        nullptr,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        return true;
    }
    else if (result < 0)
    {
        std::cerr << "Failure running 'vkAcquireNextImageKHR': " << result << std::endl;
    }

    SwapchainElement* element = elements.at(imageIndex);

    if (element->lastFence)
    {
        vkWaitForFences(ctx->vkDevice, 1, &element->lastFence, true, std::numeric_limits<uint64_t>::max());
    }
    element->lastFence = currentElement->fence;

    vkResetFences(ctx->vkDevice, 1, &currentElement->fence);

    element->draw(ubo, vertex, index, drawCount);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &currentElement->startSemaphore;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &element->commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &currentElement->endSemaphore;

    vkQueueSubmit(ctx->vkQueue, 1, &submitInfo, currentElement->fence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &currentElement->endSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(ctx->vkQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR){
        return true;
    }else if (result < 0){
        std::cerr << "Failure running 'vkQueuePresentKHR': " << result << std::endl;
    }

    currentFrame = (currentFrame + 1) % elements.size();

    return false;
}
