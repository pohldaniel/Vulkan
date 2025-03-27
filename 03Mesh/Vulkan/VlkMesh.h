#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <Vulkan/VlkBuffer.h>

class VlkMesh {

public:

	VlkMesh(const VlkBuffer& vertex, const VlkBuffer& index, const uint32_t drawCount);
	VlkMesh(VlkMesh const& rhs);
	VlkMesh(VlkMesh&& rhs);

	std::vector<VkShaderEXT> m_shader;


	void draw(const VkCommandBuffer& vkCommandBuffer, const UniformBufferObject& ubo) const;

	void setShader(std::vector<VkShaderEXT>& shader);
	void createMVP(const VmaAllocator& vmaAllocator, const VkDescriptorSet& vkDescriptorSet);

	VmaBuffer uniformMVP;
	UniformBufferObject* uniformMappingMVP;

	const VlkBuffer& vertex;
	const VlkBuffer& index;
	const uint32_t drawCount;
};