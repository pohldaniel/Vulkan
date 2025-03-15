#include <windows.h>
#include "renderer.h"
#include "device.h"
#include "shader.h"
#include "command.h"

Engine::Engine(void* window, int width, int height)  {


	std::cout << "Made a graphics engine" << std::endl;

	instance = make_instance("Real Engine", instanceDeletionQueue);
	dldi = vk::detail::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);
	
	vk::Win32SurfaceCreateInfoKHR surfaceInfo = {};
	surfaceInfo.sType = vk::StructureType::eWin32SurfaceCreateInfoKHR;
	surfaceInfo.hwnd = (HWND)window;
	surfaceInfo.hinstance = GetModuleHandleA(0);
	instance.createWin32SurfaceKHR(&surfaceInfo, 0, &surface);

	instanceDeletionQueue.push_back([this](vk::Instance instance) {
		instance.destroySurfaceKHR(surface);
	});

	physicalDevice = choose_physical_device(instance);

	logicalDevice = create_logical_device(
		physicalDevice, surface, deviceDeletionQueue);
	uint32_t graphicsQueueFamilyIndex = find_queue_family_index(
		physicalDevice, surface, vk::QueueFlagBits::eGraphics);
	graphicsQueue = logicalDevice.getQueue(graphicsQueueFamilyIndex, 0);

	swapchain.build(
		logicalDevice, physicalDevice, surface, width, height,
		deviceDeletionQueue);

	std::vector<vk::Image> images =
		logicalDevice.getSwapchainImagesKHR(swapchain.chain).value;

	for (uint32_t i = 0; i < images.size(); ++i) {
		frames.push_back(
			Frame(images[i], logicalDevice,
				swapchain.format.format, deviceDeletionQueue));
	}

	shaders = make_shader_objects(logicalDevice,
		"_shader", dldi,
		deviceDeletionQueue);

	commandPool = make_command_pool(logicalDevice, graphicsQueueFamilyIndex,
		deviceDeletionQueue);

	for (uint32_t i = 0; i < images.size(); ++i) {
		frames[i].set_command_buffer(
			allocate_command_buffer(logicalDevice, commandPool), shaders, swapchain.extent, dldi);
	}
}

void Engine::draw() {

	uint32_t imageIndex{ 0 };

	vk::SubmitInfo submitInfo = {};

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frames[0].commandBuffer;

	graphicsQueue.submit(submitInfo);

	graphicsQueue.waitIdle();

	vk::PresentInfoKHR presentInfo = {};

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain.chain;

	presentInfo.pImageIndices = &imageIndex;

	graphicsQueue.presentKHR(presentInfo);
	graphicsQueue.waitIdle();
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