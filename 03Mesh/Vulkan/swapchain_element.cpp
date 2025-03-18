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
        textureDescriptorWrite.dstBinding = 1;
        textureDescriptorWrite.descriptorCount = 1;
        textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureDescriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(
            ctx->vkDevice,
            1,
            &textureDescriptorWrite,
            0,
            nullptr
        );
    }

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

void SwapchainElement::draw()
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
        entity->draw();
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
