#include <iostream>
#include <vector>
#include <SOIL2/SOIL2.h>
#include "VlkContext.h"
#include "VlkTexture.h"

void VlkTexture::loadFromFile(std::string fileName, const bool flipVertical) {
    int numComponents, width, height;
    unsigned char* imageData = SOIL_load_image(fileName.c_str(), &width, &height, &numComponents, SOIL_LOAD_AUTO);

    if (numComponents == 3) {

        unsigned char* bytesNew = (unsigned char*)malloc(width * height * 4);

        for (unsigned int i = 0, k = 0; i < static_cast<unsigned int>(width * height * 4); i = i + 4, k = k + 3) {
            bytesNew[i] = imageData[k];
            bytesNew[i + 1] = imageData[k + 1];
            bytesNew[i + 2] = imageData[k + 2];
            bytesNew[i + 3] = 255;
        }

        SOIL_free_image_data(imageData);
        imageData = bytesNew;
        numComponents = 4;
    }

    m_width = width;
    m_height = height;
    m_channels = numComponents;

    if (flipVertical)
        FlipVertical(imageData, numComponents * width, height);

    VkDeviceSize imageSize = width * height * numComponents;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    vlkCreateBuffer(stagingBuffer, stagingBufferMemory, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vlkMapBuffer(stagingBufferMemory, reinterpret_cast<const void*>(imageData), imageSize);
    vlkCreateImage(m_vkImage, m_vkDeviceMemory, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const VkCommandBuffer& vkCommandBuffer = vlkContext.vkCommandBuffer;
    vlkBeginCommandBuffer(vkCommandBuffer);
    vlkTransitionImageLayout(vkCommandBuffer, m_vkImage, VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vlkEndCommandBuffer(vkCommandBuffer, true);
    vlkCopyBufferToImage(stagingBuffer, m_vkImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    vlkBeginCommandBuffer(vkCommandBuffer);
    vlkTransitionImageLayout(vkCommandBuffer, m_vkImage, VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vlkEndCommandBuffer(vkCommandBuffer, true);
    vkDestroyBuffer(vlkContext.vkDevice, stagingBuffer, nullptr);
    vkFreeMemory(vlkContext.vkDevice, stagingBufferMemory, nullptr);

    vlkCreateImageView(m_vkImageView, m_vkImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, { VK_COMPONENT_SWIZZLE_IDENTITY , VK_COMPONENT_SWIZZLE_IDENTITY , VK_COMPONENT_SWIZZLE_IDENTITY , VK_COMPONENT_SWIZZLE_IDENTITY });
    //vlkAllocateDescriptorSets(m_vkDescriptorSet);
   
    //bind(1u);
}

void VlkTexture::bind(uint32_t dstBinding) {  
    //VkDescriptorSetAllocateInfo allocInfo{};
    //allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    //allocInfo.descriptorPool = vlkContext.descriptorPool;
    //allocInfo.descriptorSetCount = 1;
    //allocInfo.pSetLayouts = &vlkContext.descriptorSetLayout;

    //vkAllocateDescriptorSets(vlkContext.vkDevice, &allocInfo, &m_vkDescriptorSet);
    //createMVP();
    //vlkBindImageViewToDescriptorSet(m_vkImageView, m_vkDescriptorSet, dstBinding);
}

void VlkTexture::setDescriptorSet(VkDescriptorSet vkDescriptorSet) {
    m_vkDescriptorSet = vkDescriptorSet;
}

void VlkTexture::FlipVertical(unsigned char* data, unsigned int padWidth, unsigned int height) {
    std::vector<unsigned char> srcPixels(padWidth * height);
    memcpy(&srcPixels[0], data, padWidth * height);
    unsigned char* pSrcRow = 0;
    unsigned char* pDestRow = 0;
    for (unsigned int i = 0; i < height; ++i) {
        pSrcRow = &srcPixels[(height - 1 - i) * padWidth];
        pDestRow = &data[i * padWidth];
        memcpy(pDestRow, pSrcRow, padWidth);
    }
}

void VlkTexture::createMVP() {
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = sizeof(UniformBufferObject);
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vmaCreateBuffer(vlkContext.memoryAllocator, &createInfo, &allocInfo, &uniformMVP.buffer, &uniformMVP.allocation, nullptr);

    vmaMapMemory(vlkContext.memoryAllocator, uniformMVP.allocation, reinterpret_cast<void**>(&uniformMappingMVP));

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
    descriptorWrite.dstSet = m_vkDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 1;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(vlkContext.vkDevice, 1, &descriptorWrite, 0, nullptr);
}