#pragma once

#include <vulkan/vulkan.h>

class VlkBuffer {
public:

	VlkBuffer();
	VlkBuffer(VlkBuffer const& rhs);
	VlkBuffer(VlkBuffer&& rhs);


	void createBufferVertex(const void* data, uint32_t size);
	void createBufferIndex(const void* data, uint32_t size);

	VkBuffer m_vkBuffer;
	VkDeviceMemory m_vkDeviceMemory;
};