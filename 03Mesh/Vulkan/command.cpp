#include "command.h"

VkCommandPool make_command_pool(VkDevice logicalDevice, uint32_t queueFamilyIndex,
	std::deque<std::function<void(VkDevice)>>& deletionQueue) {

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags  = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndex;

	VkCommandPool pool = nullptr;

	auto result = vkCreateCommandPool(logicalDevice, &poolInfo, 0, &pool);
	if (result == VkResult::VK_SUCCESS) {
		deletionQueue.push_back([pool](VkDevice device) {
			vkDestroyCommandPool(device, pool, NULL);
		});
	}else {
		std::cout << "Command pool creation failed" << std::endl;
	}
	return pool;
}

VkCommandBuffer allocate_command_buffer(VkDevice logicalDevice, VkCommandPool commandPool) {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = 1;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	VkCommandBuffer buffer;
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &buffer);
	return buffer;
}