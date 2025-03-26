#include <Vulkan/VlkContext.h>
#include "Data.h"
#include "VlkMesh.h"

void VlkMesh::draw(const VkCommandBuffer& vkCommandBuffer, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount) {
    // Mesh
    VkDeviceSize meshOffset = 0;
    vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, &vertex, &meshOffset);
    vkCmdBindIndexBuffer(vkCommandBuffer, index, 0, VK_INDEX_TYPE_UINT32);

    // Shader
    VkShaderStageFlagBits stages[] = {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT
    };

    vkCmdBindShadersEXT(vkCommandBuffer, sizeof(stages) / sizeof(VkShaderStageFlagBits), stages, m_shader.data());

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

    vkCmdSetVertexInputEXT(vkCommandBuffer, 1, &vertexBinding, sizeof(vertexAttributes) / sizeof(VkVertexInputAttributeDescription2EXT), vertexAttributes);
    // Input assembly settings
    vkCmdSetPrimitiveTopology(vkCommandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkCmdSetPrimitiveRestartEnable(vkCommandBuffer, false);
    // Multisample settings
    vkCmdSetRasterizationSamplesEXT(vkCommandBuffer, VK_SAMPLE_COUNT_1_BIT);
    VkSampleMask sampleMask = 1;
    vkCmdSetSampleMaskEXT(vkCommandBuffer, VK_SAMPLE_COUNT_1_BIT, &sampleMask);
    vkCmdSetAlphaToCoverageEnableEXT(vkCommandBuffer, false);

    // Color blend settings
    VkBool32 colorBlend = true;
    vkCmdSetColorBlendEnableEXT(vkCommandBuffer, 0, 1, &colorBlend);
    VkColorBlendEquationEXT colorBlendEquation{};
    colorBlendEquation.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendEquation.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendEquation.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendEquation.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendEquation.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendEquation.alphaBlendOp = VK_BLEND_OP_ADD;
    vkCmdSetColorBlendEquationEXT(vkCommandBuffer, 0, 1, &colorBlendEquation);
    VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    vkCmdSetColorWriteMaskEXT(vkCommandBuffer, 0, 1, &colorWriteMask);
}

void VlkMesh::setShader(std::vector<VkShaderEXT>& shader) {
    m_shader = shader;
}

void VlkMesh::createMVP(const VmaAllocator& vmaAllocator, const VkDescriptorSet& vkDescriptorSet) {
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = sizeof(UniformBufferObject);
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vmaCreateBuffer(vmaAllocator, &createInfo, &allocInfo, &uniformMVP.buffer, &uniformMVP.allocation, nullptr);
    vmaMapMemory(vmaAllocator, uniformMVP.allocation, reinterpret_cast<void**>(&uniformMappingMVP));

    uniformMappingMVP->proj = glm::mat4(1.0f);
    uniformMappingMVP->view = glm::mat4(1.0f);
    uniformMappingMVP->model = glm::mat4(1.0f);

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformMVP.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.dstSet = vkDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 1;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(vlkContext.vkDevice, 1, &descriptorWrite, 0, nullptr);
}