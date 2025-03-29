#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <Vulkan/Data.h>

class VlkTexture {

	friend class VlkSwapchainElement;

public:

	VlkTexture();
	VlkTexture(VlkTexture const& rhs);
	VlkTexture(VlkTexture&& rhs) noexcept;

	void loadFromFile(std::string fileName, const bool flipVertical = false);	
	void bind(uint32_t dstBinding);

private:

	void allocateDescriptorSet(const VkDescriptorSetLayout& vkDescriptorSetLayout);

	VkImage m_vkImage = VK_NULL_HANDLE;
	VkDeviceMemory m_vkDeviceMemory = VK_NULL_HANDLE;
	VkImageView m_vkImageView = VK_NULL_HANDLE;
	VkDescriptorSet m_vkDescriptorSet = VK_NULL_HANDLE;

	unsigned int m_width = 0u;
	unsigned int m_height = 0u;
	unsigned short m_channels = 0u;

	static void FlipVertical(unsigned char* data, unsigned int padWidth, unsigned int height);
};