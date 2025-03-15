#include "command.h"

vk::CommandPool make_command_pool(vk::Device logicalDevice, uint32_t queueFamilyIndex,
	std::deque<std::function<void(vk::Device)>>& deletionQueue) {

	vk::CommandPoolCreateInfo poolInfo = {};
	poolInfo.setFlags(vk::CommandPoolCreateFlags()
		| vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	poolInfo.setQueueFamilyIndex(queueFamilyIndex);

	vk::CommandPool pool = nullptr;

	auto result = logicalDevice.createCommandPool(poolInfo);
	if (result.result == vk::Result::eSuccess) {
		std::cout << "Command pool created successfully" << std::endl;
		pool = result.value;
		deletionQueue.push_back([pool](vk::Device device) {
			device.destroyCommandPool(pool);
			std::cout << "Destroyed command pool!" << std::endl;
			});
	}
	else {
		std::cout << "Command pool creation failed" << std::endl;
	}
	return pool;
}

vk::CommandBuffer allocate_command_buffer(vk::Device logicalDevice, vk::CommandPool commandPool) {
	vk::CommandBufferAllocateInfo allocInfo = {};
	allocInfo.setCommandBufferCount(1);
	allocInfo.setCommandPool(commandPool);
	allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
	return logicalDevice.allocateCommandBuffers(allocInfo).value[0];
}