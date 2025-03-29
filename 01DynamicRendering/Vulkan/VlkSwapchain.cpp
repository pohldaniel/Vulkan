#include <iostream>
#include "VlkSwapchain.h"
#include "VlkContext.h"
#include "VlkTexture.h"

VlkSwapchain::VlkSwapchain(VlkContext* ctx, unsigned width, unsigned height, const VkPresentModeKHR vkPresentModeKHR, VkSwapchainKHR vkOldSwapchainKHR)
    : ctx(ctx)
    , width(width)
    , height(height)
    , currentFrame(0)
    , imageIndex(0)
    , imageCount(0)
    , swapchain(VK_NULL_HANDLE)
{
    
    vlkCreateSwapChain(swapchain, format, width, height, vkPresentModeKHR, vkOldSwapchainKHR);
    vkGetSwapchainImagesKHR(ctx->vkDevice, swapchain, &imageCount, VK_NULL_HANDLE);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(ctx->vkDevice, swapchain, &imageCount, images.data());

    depthImages.reserve(imageCount);
    depthImagesMemory.reserve(imageCount);

    for (uint32_t i = 0; i < imageCount; i++) {
        vlkCreateImage(depthImages[i], depthImagesMemory[i], width, height, ctx->vkDepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        elements.push_back(new VlkSwapchainElement(this, images[i], depthImages[i]));
    }
}

VlkSwapchain::~VlkSwapchain(){
    for (VlkSwapchainElement* element : elements){
        delete element;
    }

    for (uint32_t i = 0; i < imageCount; i++) {
        vlkDestroyImage(depthImages[i], depthImagesMemory[i]);
    }
    vkDestroySwapchainKHR(ctx->vkDevice, swapchain, VK_NULL_HANDLE);
}

bool VlkSwapchain::draw(const std::list<VlkMesh>& meshes){
    VkResult result;
 
    const VlkSwapchainElement* currentElement = elements.at(currentFrame);

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

    VlkSwapchainElement* element = elements.at(imageIndex);

    if (element->lastFence)
    {
        vkWaitForFences(ctx->vkDevice, 1, &element->lastFence, true, std::numeric_limits<uint64_t>::max());
    }
    element->lastFence = currentElement->fence;

    vkResetFences(ctx->vkDevice, 1, &currentElement->fence);

    element->draw(meshes);

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