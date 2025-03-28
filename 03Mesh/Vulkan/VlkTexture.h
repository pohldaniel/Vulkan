#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <Vulkan/Data.h>

class VlkTexture {

public:

	void loadFromFile(std::string fileName, const bool flipVertical = false);
	void bind(uint32_t dstBinding);
	void setDescriptorSet(VkDescriptorSet vkDescriptorSet);
	void createMVP();

	VkImage m_vkImage = VK_NULL_HANDLE;
	VkDeviceMemory m_vkDeviceMemory = VK_NULL_HANDLE;
	VkImageView m_vkImageView = VK_NULL_HANDLE;
	mutable VkDescriptorSet m_vkDescriptorSet = VK_NULL_HANDLE;

	unsigned int m_width = 0u;
	unsigned int m_height = 0u;
	unsigned short m_channels = 0u;
	VmaBuffer uniformMVP;
	UniformBufferObject* uniformMappingMVP;

	static void FlipVertical(unsigned char* data, unsigned int padWidth, unsigned int height);
};