#include <iostream>
#include <imgui_impl_vulkan.h>
#include "VlkContext.h"
#include "VlkSwapchain.h"
#include "VlkSwapchainElement.h"
#include "VlkMesh.h"
#include "VlkTexture.h"

VlkSwapchainElement::VlkSwapchainElement(VlkSwapchain* swapchain, VkImage image, VkImage depthImage) : ctx(swapchain->ctx), image(image), depthImage(depthImage), swapchain(swapchain), descriptorIndex(nextUniformIndex++) {

    vlkCreateCommandBuffer(commandBuffer);
    vlkCreateSemaphore(startSemaphore);
    vlkCreateSemaphore(endSemaphore);
    vlkCreateFence(fence);

    vlkCreateImageView(imageView, image, swapchain->format, VK_IMAGE_ASPECT_COLOR_BIT, { VK_COMPONENT_SWIZZLE_IDENTITY , VK_COMPONENT_SWIZZLE_IDENTITY , VK_COMPONENT_SWIZZLE_IDENTITY , VK_COMPONENT_SWIZZLE_IDENTITY });
    vlkCreateImageView(depthImageView, depthImage, ctx->vkDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    {
        colorAttachment = {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = imageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = { { 1.0f, 1.0f, 1.0f, 1.0f } };

        depthStencilAttachment = {};
        depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthStencilAttachment.imageView = depthImageView;
        depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthStencilAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
        depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthStencilAttachment.clearValue.depthStencil.depth = 1.0f;
        depthStencilAttachment.clearValue.depthStencil.stencil = 0;

        renderingInfo = {};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = { { 0, 0 },{ swapchain->width, swapchain->height } };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthStencilAttachment;
        renderingInfo.pStencilAttachment = &depthStencilAttachment;

        viewport = {};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(swapchain->height);
        viewport.width = static_cast<float>(swapchain->width);
        viewport.height = -static_cast<float>(swapchain->height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        scissor = { { 0, 0 },{ swapchain->width, swapchain->height } };
    }
}

VlkSwapchainElement::~VlkSwapchainElement(){
    vkFreeCommandBuffers(ctx->vkDevice, ctx->vkCommandPool, 1, &commandBuffer);
    vkDestroySemaphore(ctx->vkDevice, startSemaphore, nullptr);
    vkDestroySemaphore(ctx->vkDevice, endSemaphore, nullptr);
    vkDestroyFence(ctx->vkDevice, fence, nullptr);
    vkDestroyImageView(ctx->vkDevice, imageView, nullptr);
    vkDestroyImageView(ctx->vkDevice, depthImageView, nullptr);
}

void VlkSwapchainElement::draw(const UniformBufferObject& ubo, const std::list<VlkMesh*>& meshes, std::vector<VlkTexture>& textures){
   
    
    vlkBeginCommandBuffer(commandBuffer);

    vlkTransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT,
                             VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);

    vlkTransitionImageLayout(commandBuffer, depthImage, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                             VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                             VK_ACCESS_NONE, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
      
    // Begin rendering   
    vkCmdBeginRendering(commandBuffer, &renderingInfo);
  
    vkCmdSetViewportWithCount(commandBuffer, 1, &viewport);
    vkCmdSetScissorWithCount(commandBuffer, 1, &scissor);
    vkCmdSetPolygonModeEXT(commandBuffer, ctx->vkPolygonMode);
    vkCmdSetDepthWriteEnable(commandBuffer, VK_TRUE);
    vkCmdSetDepthTestEnable(commandBuffer, VK_TRUE);
    vkCmdSetStencilTestEnable(commandBuffer, VK_FALSE);
    vkCmdSetDepthCompareOp(commandBuffer, VK_COMPARE_OP_LESS_OR_EQUAL);
    vkCmdSetRasterizerDiscardEnable(commandBuffer, false);
    vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_NONE);
    vkCmdSetDepthBiasEnable(commandBuffer, false);
    vkCmdSetLineWidth(commandBuffer, 2.0f);
 
    // Uniform
    vlkContext.uniformMappingMVP->model = ubo.model;
    vlkContext.uniformMappingMVP->view = ubo.view;
    vlkContext.uniformMappingMVP->proj = ubo.proj;
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipelineLayout, 0, 1, &vlkContext.m_vkDescriptorSet, 0, NULL);

    // Bind global descriptor set
    for (std::pair<std::list<VlkMesh*>::const_iterator, std::vector<VlkTexture>::iterator> i(meshes.begin(), textures.begin());
        i.first != meshes.end(); ++i.first, ++i.second) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipelineLayout, 1, 1, &(*i.second).m_vkDescriptorSet, 0, NULL);
        draw(commandBuffer, ubo, (*i.first)->vertex.m_vkBuffer, (*i.first)->index.m_vkBuffer, (*i.first)->drawCount, &(*i.second));
    }
    
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    // End rendering
    vkCmdEndRendering(commandBuffer);

    vlkTransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                             VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    
    vlkTransitionImageLayout(commandBuffer, depthImage, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                             VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_NONE,
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    vlkEndCommandBuffer(commandBuffer);
    //vlkQueueSubmit(commandBuffer);
}

void VlkSwapchainElement::draw(const VkCommandBuffer& vkCommandbuffer, const UniformBufferObject& ubo, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount, VlkTexture* texture) {

    // Mesh
    VkDeviceSize meshOffset = 0;
    vkCmdBindVertexBuffers(vkCommandbuffer, 0, 1, &vertex, &meshOffset);
    vkCmdBindIndexBuffer(vkCommandbuffer, index, 0, VK_INDEX_TYPE_UINT32);

    // Shader
    VkShaderStageFlagBits stages[] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };

    vkCmdBindShadersEXT(vkCommandbuffer, sizeof(stages) / sizeof(VkShaderStageFlagBits), stages, ctx->shader.data());

    // Vertex input settings
    VkVertexInputBindingDescription2EXT vertexBinding{};
    vertexBinding.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
    vertexBinding.binding = 0;
    vertexBinding.stride = sizeof(float) * 8;
    vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexBinding.divisor = 1;

    VkVertexInputAttributeDescription2EXT vertexAttributes[3]{};
    vertexAttributes[0].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
    vertexAttributes[0].location = 0;
    vertexAttributes[0].binding = 0;
    vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[0].offset = 0;

    vertexAttributes[1].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
    vertexAttributes[1].location = 1;
    vertexAttributes[1].binding = 0;
    vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributes[1].offset = 3 * sizeof(float);

    vertexAttributes[2].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
    vertexAttributes[2].location = 2;
    vertexAttributes[2].binding = 0;
    vertexAttributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[2].offset = 5 * sizeof(float);

    vkCmdSetVertexInputEXT(vkCommandbuffer, 1, &vertexBinding, sizeof(vertexAttributes) / sizeof(VkVertexInputAttributeDescription2EXT), vertexAttributes);

    // Input assembly settings
    vkCmdSetPrimitiveTopology(vkCommandbuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkCmdSetPrimitiveRestartEnable(vkCommandbuffer, false);

    // Multisample settings
    vkCmdSetRasterizationSamplesEXT(vkCommandbuffer, VK_SAMPLE_COUNT_1_BIT);
    VkSampleMask sampleMask = 1;
    vkCmdSetSampleMaskEXT(vkCommandbuffer, VK_SAMPLE_COUNT_1_BIT, &sampleMask);
    vkCmdSetAlphaToCoverageEnableEXT(vkCommandbuffer, false);

    // Color blend settings
    VkBool32 colorBlend = false;
    vkCmdSetColorBlendEnableEXT(vkCommandbuffer, 0, 1, &colorBlend);
    VkColorBlendEquationEXT colorBlendEquation{};
    colorBlendEquation.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendEquation.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendEquation.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendEquation.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendEquation.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendEquation.alphaBlendOp = VK_BLEND_OP_ADD;
    vkCmdSetColorBlendEquationEXT(vkCommandbuffer, 0, 1, &colorBlendEquation);
    VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    vkCmdSetColorWriteMaskEXT(vkCommandbuffer, 0, 1, &colorWriteMask);

    

    // Push constants
    int ids[] = { descriptorIndex,0 };
    vkCmdPushConstants(vkCommandbuffer, ctx->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ids), ids);
    vkCmdDrawIndexed(vkCommandbuffer, drawCount, 1, 0, 0, 0);
}
