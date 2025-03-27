#include <Vulkan/VlkContext.h>
#include "VlkBuffer.h"

VlkBuffer::VlkBuffer(): m_vkBuffer(VK_NULL_HANDLE), m_vkDeviceMemory(VK_NULL_HANDLE) {

}

VlkBuffer::VlkBuffer(VlkBuffer const& rhs) : m_vkBuffer (rhs.m_vkBuffer), m_vkDeviceMemory(rhs.m_vkDeviceMemory) {

}

VlkBuffer::VlkBuffer(VlkBuffer&& rhs) : m_vkBuffer(rhs.m_vkBuffer), m_vkDeviceMemory(rhs.m_vkDeviceMemory) {

}

void VlkBuffer::createBufferVertex(const void* data, uint32_t size) {
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	vlkCreateBuffer(stagingBuffer, stagingBufferMemory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vlkMapBuffer(stagingBufferMemory, data, size);
	vlkCreateBuffer(m_vkBuffer, m_vkDeviceMemory, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vlkCopyBuffer(stagingBuffer, m_vkBuffer, size);

	vkDestroyBuffer(vlkContext.vkDevice, stagingBuffer, VK_NULL_HANDLE);
	vkFreeMemory(vlkContext.vkDevice, stagingBufferMemory, VK_NULL_HANDLE);
}

void VlkBuffer::createBufferIndex(const void* data, uint32_t size) {
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	vlkCreateBuffer(stagingBuffer, stagingBufferMemory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vlkMapBuffer(stagingBufferMemory, data, size);
	vlkCreateBuffer(m_vkBuffer, m_vkDeviceMemory, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vlkCopyBuffer(stagingBuffer, m_vkBuffer, size);

	vkDestroyBuffer(vlkContext.vkDevice, stagingBuffer, VK_NULL_HANDLE);
	vkFreeMemory(vlkContext.vkDevice, stagingBufferMemory, VK_NULL_HANDLE);
}