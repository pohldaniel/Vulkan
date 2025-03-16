#include <windows.h>
#include "renderer.h"
#include "device.h"
#include "shader.h"
#include "command.h"

Engine::Engine(void* window, int width, int height)  {


	std::cout << "Made a graphics engine" << std::endl;

	instance = make_instance("Real Engine", instanceDeletionQueue);
	//dldi = vk::detail::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);
	
	VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
	surfaceInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.hwnd = (HWND)window;
	surfaceInfo.hinstance = GetModuleHandleA(0);
	//instance.createWin32SurfaceKHR(&surfaceInfo, 0, &surface);

	vkCreateWin32SurfaceKHR(instance, &surfaceInfo, 0, &surface);

	instanceDeletionQueue.push_back([this](VkInstance instance) {
		vkDestroySurfaceKHR(instance, surface, NULL);
	});

	physicalDevice = choose_physical_device(instance);

	logicalDevice = create_logical_device(physicalDevice, surface, deviceDeletionQueue);
	graphicsQueueFamilyIndex = find_queue_family_index(physicalDevice, surface, VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT);
	vkGetDeviceQueue(logicalDevice, graphicsQueueFamilyIndex, 0, &graphicsQueue);


	swapchain.build(logicalDevice, physicalDevice, surface, width, height,deviceDeletionQueue);

	//std::vector<VkImage> images = logicalDevice.getSwapchainImagesKHR(swapchain.chain).value;
	uint32_t scImgCount = 0;
	vkGetSwapchainImagesKHR(logicalDevice, swapchain.chain, &scImgCount, 0);
	VkImage* images = new VkImage[scImgCount];
	vkGetSwapchainImagesKHR(logicalDevice, swapchain.chain, &scImgCount, images);

	for (uint32_t i = 0; i < scImgCount; ++i) {
		frames.push_back(
			Frame(images[i], logicalDevice,
				swapchain.format.format, deviceDeletionQueue));
	}

	shaders = make_shader_objects(instance, logicalDevice,"_shader", deviceDeletionQueue);

	commandPool = make_command_pool(logicalDevice, graphicsQueueFamilyIndex,deviceDeletionQueue);

	for (uint32_t i = 0; i < scImgCount; ++i) {
		frames[i].set_command_buffer(instance, allocate_command_buffer(logicalDevice, commandPool), shaders, swapchain.extent);
	}
}

void Engine::draw() {

	uint32_t imageIndex = 0;

	//vkAcquireNextImageKHR(logicalDevice, swapchain.chain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frames[0].commandBuffer;


	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain.chain;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(graphicsQueue, &presentInfo);
	vkQueueWaitIdle(graphicsQueue);
}

Engine::~Engine() {

	while (deviceDeletionQueue.size() > 0) {
		deviceDeletionQueue.back()(logicalDevice);
		deviceDeletionQueue.pop_back();
	}

	while (instanceDeletionQueue.size() > 0) {
		instanceDeletionQueue.back()(instance);
		instanceDeletionQueue.pop_back();
	}
}