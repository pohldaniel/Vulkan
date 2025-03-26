#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class VlkMesh {

public:

	std::vector<VkShaderEXT> m_shader;


	void draw(const VkCommandBuffer& vkCommandBuffer, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount);

	void setShader(std::vector<VkShaderEXT>& shader);
	void createMVP(const VmaAllocator& vmaAllocator, const VkDescriptorSet& vkDescriptorSet);

	VmaBuffer uniformMVP;
	UniformBufferObject* uniformMappingMVP;
};