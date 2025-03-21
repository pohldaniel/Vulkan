#include "Vulkan/VkContext.h"
#include "swapchain_element.h"
#include "swap_chain.h"

SwapchainElement::SwapchainElement(Swapchain* swapchain, VkImage image)
    : ctx(swapchain->ctx)
    , image(image)
    , swapchain(swapchain)
{
    VkResult result;

    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        createInfo.image = image;
        createInfo.format = swapchain->format;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkCreateImageView(ctx->vkDevice, &createInfo, nullptr, &imageView);
    }

    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = ctx->vkCommandPool;
        allocInfo.commandBufferCount = 1;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(ctx->vkDevice, &allocInfo, &commandBuffer);
    }

    {
        VkSemaphoreCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vkCreateSemaphore(ctx->vkDevice, &createInfo, nullptr, &startSemaphore);
    }

    {
        VkSemaphoreCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vkCreateSemaphore(ctx->vkDevice, &createInfo, nullptr, &endSemaphore);
    }

    {
        VkFenceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        vkCreateFence(ctx->vkDevice, &createInfo, nullptr, &fence);
    }

    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = ctx->descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &ctx->descriptorSetLayout;

        vkAllocateDescriptorSets(ctx->vkDevice, &allocInfo, &descriptorSet);
    }

    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = ctx->sampler;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = ctx->textureView;

        VkWriteDescriptorSet textureDescriptorWrite{};
        textureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureDescriptorWrite.dstSet = descriptorSet;
        textureDescriptorWrite.dstBinding = 2;
        textureDescriptorWrite.descriptorCount = 1;
        textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureDescriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(ctx->vkDevice, 1, &textureDescriptorWrite, 0, nullptr);
    }

    createImage(swapchain->width, swapchain->height, ctx->vkDepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    createImageView(depthImage, ctx->vkDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depthImageView);

    //transitionImageLayout(depthImage, ctx->vkDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    static const float positions[4][2] = {
        { -0.5, -0.25 },
        { -0.5, +0.25 },
        { +0.5, +0.25 },
        { +0.5, -0.25 }
    };

    for (int i = 0; i < 4; i++)
    {
        entities.push_back(new Entity(this, positions[i][0], positions[i][1]));
    }
}

SwapchainElement::~SwapchainElement()
{
    for (Entity* entity : entities)
    {
        delete entity;
    }

    vkFreeDescriptorSets(ctx->vkDevice, ctx->descriptorPool, 1, &descriptorSet);

    vkDestroyFence(ctx->vkDevice, fence, nullptr);
    vkDestroySemaphore(ctx->vkDevice, endSemaphore, nullptr);
    vkDestroySemaphore(ctx->vkDevice, startSemaphore, nullptr);
    vkFreeCommandBuffers(ctx->vkDevice, ctx->vkCommandPool, 1, &commandBuffer);

    vkDestroyImageView(ctx->vkDevice, imageView, nullptr);
}

void SwapchainElement::draw(const UniformBufferObject& ubo, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount)
{
    VkResult result;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if (!ctx->textureReady)
    {
        prepareTexture();
        ctx->textureReady = true;
    }

    imageToAttachmentLayout();

    // Begin rendering
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = imageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {
        { 0, 0 },
        { swapchain->width, swapchain->height }
    };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    // Set render size
    VkViewport viewport = {
        0, static_cast<float>(swapchain->height),
        static_cast<float>(swapchain->width), -static_cast<float>(swapchain->height),
        0, 1
    };
    vkCmdSetViewportWithCount(commandBuffer, 1, &viewport);

    VkRect2D scissor = {
        { 0, 0 },
        { swapchain->width, swapchain->height }
    };
    vkCmdSetScissorWithCount(commandBuffer, 1, &scissor);

    // Bind global descriptor set
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        ctx->pipelineLayout,
        0,
        1,
        &descriptorSet,
        0,
        nullptr
    );

    // Draw entities
    for (Entity* entity : entities)
    {
        entity->draw(ubo, vertex, index, drawCount);
    }

    // End rendering
    vkCmdEndRendering(commandBuffer);

    imageToPresentLayout();

    vkEndCommandBuffer(commandBuffer);
}

void SwapchainElement::prepareTexture()
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.image = ctx->vmaImage.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    vkCmdPipelineBarrier(
        commandBuffer, 
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );
}

void SwapchainElement::imageToAttachmentLayout()
{
    VkImageMemoryBarrier beforeBarrier{};
    beforeBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    beforeBarrier.srcAccessMask = VK_ACCESS_NONE;
    beforeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    beforeBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    beforeBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    beforeBarrier.image = image;
    beforeBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    beforeBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    beforeBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &beforeBarrier
    );
}

void SwapchainElement::imageToPresentLayout()
{
    VkImageMemoryBarrier afterBarrier{};
    afterBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    afterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    afterBarrier.dstAccessMask = VK_ACCESS_NONE;
    afterBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    afterBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    afterBarrier.image = image;
    afterBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    afterBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    afterBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &afterBarrier
    );
}

void SwapchainElement::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& vkImageView) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(ctx->vkDevice, &viewInfo, nullptr, &vkImageView);
}

void SwapchainElement::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateImage(ctx->vkDevice, &imageInfo, nullptr, &image);
   
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(ctx->vkDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = ctx->findMemoryType(memRequirements.memoryTypeBits, properties);

    vkAllocateMemory(ctx->vkDevice, &allocInfo, nullptr, &imageMemory);
    

    vkBindImageMemory(ctx->vkDevice, image, imageMemory, 0);
}

void SwapchainElement::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    const VkCommandBuffer& vkCommandBuffer = ctx->vkCommandBuffer;

    VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
    vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkCommandBufferBeginInfo.pNext = NULL;
    vkCommandBufferBeginInfo.pInheritanceInfo = NULL;

    vkBeginCommandBuffer(vkCommandBuffer, &vkCommandBufferBeginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
   

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vkEndCommandBuffer(vkCommandBuffer);
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCommandBuffer;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = VK_NULL_HANDLE;
    submitInfo.pWaitDstStageMask = VK_NULL_HANDLE;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = VK_NULL_HANDLE;
    submitInfo.pNext = NULL;

    vkQueueSubmit(ctx->vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx->vkQueue);
}