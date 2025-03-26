#pragma once

#include <vulkan/vulkan.h>

class VlkBuffer {
public:
	void createBuffer(const void* data, uint32_t size);

	VkBuffer m_vkBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_vkDeviceMemory = VK_NULL_HANDLE;
};