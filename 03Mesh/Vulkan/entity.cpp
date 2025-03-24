#include "VlkExtension.h"
#include "VlkContext.h"
#include "VlkSwapchain.h"
#include "VlkSwapchainElement.h"
#include "entity.h"

Entity::Entity(VlkSwapchainElement* element, float x, float y)
    : ctx(element->ctx)
    , element(element)
    , descriptorIndex(element->nextUniformIndex++)
{
    VkResult result;

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = 5 * sizeof(float);
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vmaCreateBuffer(ctx->memoryAllocator,&createInfo,&allocInfo,&uniform.buffer, &uniform.allocation,nullptr);

    vmaMapMemory(ctx->memoryAllocator, uniform.allocation, reinterpret_cast<void**>(&uniformMapping));

    uniformMapping->x = x;
    uniformMapping->y = y;
    uniformMapping->rotation = 0;
    uniformMapping->scale = 0.1;
    uniformMapping->aspect = element->swapchain->width / static_cast<float>(element->swapchain->height);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniform.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.dstSet = element->descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = descriptorIndex;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(ctx->vkDevice, 1, &descriptorWrite, 0, nullptr);

    createMVP();
}

void Entity::createMVP() {
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = sizeof(UniformBufferObject);
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vmaCreateBuffer(ctx->memoryAllocator, &createInfo, &allocInfo, &uniformMVP.buffer, &uniformMVP.allocation, nullptr);

    vmaMapMemory(ctx->memoryAllocator, uniformMVP.allocation, reinterpret_cast<void**>(&uniformMappingMVP));

    uniformMappingMVP->proj = glm::mat4(1.0f);
    uniformMappingMVP->view = glm::mat4(1.0f);
    uniformMappingMVP->model = glm::mat4(1.0f);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformMVP.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.dstSet = element->descriptorSet;
    descriptorWrite.dstBinding = 1;
    descriptorWrite.dstArrayElement = descriptorIndex;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(ctx->vkDevice, 1, &descriptorWrite, 0, nullptr);
}

Entity::~Entity()
{
    vmaUnmapMemory(ctx->memoryAllocator, uniform.allocation);
    vmaDestroyBuffer(ctx->memoryAllocator, uniform.buffer, uniform.allocation);
}

void Entity::draw(const UniformBufferObject& ubo, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount) {

    // Mesh
    VkDeviceSize meshOffset = 0;
    vkCmdBindVertexBuffers(element->commandBuffer, 0, 1, &vertex, &meshOffset);
    vkCmdBindIndexBuffer(element->commandBuffer, index, 0, VK_INDEX_TYPE_UINT32);

    // Shader
    VkShaderStageFlagBits stages[] = {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT
    };

    vkCmdBindShadersEXT(element->commandBuffer,sizeof(stages) / sizeof(VkShaderStageFlagBits),stages,ctx->shader.data());

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

    vkCmdSetVertexInputEXT(element->commandBuffer, 1, &vertexBinding, sizeof(vertexAttributes) / sizeof(VkVertexInputAttributeDescription2EXT),vertexAttributes);

    // Input assembly settings
    vkCmdSetPrimitiveTopology(element->commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkCmdSetPrimitiveRestartEnable(element->commandBuffer, false);

   

    // Multisample settings
    vkCmdSetRasterizationSamplesEXT(element->commandBuffer, VK_SAMPLE_COUNT_1_BIT);
    VkSampleMask sampleMask = 1;
    vkCmdSetSampleMaskEXT(element->commandBuffer, VK_SAMPLE_COUNT_1_BIT, &sampleMask);
    vkCmdSetAlphaToCoverageEnableEXT(element->commandBuffer, false);

    // Color blend settings
    VkBool32 colorBlend = true;
    vkCmdSetColorBlendEnableEXT(element->commandBuffer, 0, 1, &colorBlend);
    VkColorBlendEquationEXT colorBlendEquation{};
    colorBlendEquation.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendEquation.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendEquation.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendEquation.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendEquation.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendEquation.alphaBlendOp = VK_BLEND_OP_ADD;
    vkCmdSetColorBlendEquationEXT(element->commandBuffer, 0, 1, &colorBlendEquation);
    VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    vkCmdSetColorWriteMaskEXT(element->commandBuffer, 0, 1, &colorWriteMask);

    // Uniform
    uniformMapping->rotation += 0.01f;

    uniformMappingMVP->model  = ubo.model;
    uniformMappingMVP->view = ubo.view;
    uniformMappingMVP->proj = ubo.proj;

    // Push constants
    int ids[] = {
        descriptorIndex,
        0
    };

    vkCmdPushConstants(
        element->commandBuffer,
        ctx->pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(ids),
        ids
    );

    vkCmdDrawIndexed(element->commandBuffer, drawCount, 1, 0, 0, 0);
}
