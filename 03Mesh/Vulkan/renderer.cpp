#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <windows.h>
#include "renderer.h"
#include "device.h"

#include "shader.h"
#include "command.h"

Engine::Engine(void* window, int width, int height)  {

	waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	instance = make_instance("Real Engine", instanceDeletionQueue);

	VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
	surfaceInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.hwnd = (HWND)window;
	surfaceInfo.hinstance = GetModuleHandleA(0);
	vkCreateWin32SurfaceKHR(instance, &surfaceInfo, 0, &surface);

	instanceDeletionQueue.push_back([this](VkInstance instance) {
		vkDestroySurfaceKHR(instance, surface, NULL);
	});

	physicalDevice = choose_physical_device(instance);

	logicalDevice = create_logical_device(physicalDevice, surface, deviceDeletionQueue);
	graphicsQueueFamilyIndex = find_queue_family_index(physicalDevice, surface, VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT);
	vkGetDeviceQueue(logicalDevice, graphicsQueueFamilyIndex, 0, &graphicsQueue);


	swapchain.build(logicalDevice, physicalDevice, surface, width, height,deviceDeletionQueue);

	uint32_t scImgCount = 0;
	vkGetSwapchainImagesKHR(logicalDevice, swapchain.chain, &scImgCount, 0);
	VkImage* images = new VkImage[scImgCount];
	vkGetSwapchainImagesKHR(logicalDevice, swapchain.chain, &scImgCount, images);

	for (uint32_t i = 0; i < scImgCount; ++i) {
		frames.push_back(Frame(images[i], logicalDevice, swapchain.format.format, deviceDeletionQueue));
	}

	shaders = make_shader_objects(instance, logicalDevice,"_shader", deviceDeletionQueue);

	commandPool = make_command_pool(logicalDevice, graphicsQueueFamilyIndex,deviceDeletionQueue);

	for (uint32_t i = 0; i < scImgCount; ++i) {
		frames[i].set_command_buffer(instance, allocate_command_buffer(logicalDevice, commandPool), shaders, swapchain.extent);
	}

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
	vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore);

    createAllocator();
    createMesh();
    createTexture();
}

void Engine::draw() {

	vkAcquireNextImageKHR(logicalDevice, swapchain.chain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frames[imageIndex].commandBuffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain.chain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore;

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

void Engine::createAllocator(){
    VkResult result;

    VmaAllocatorCreateInfo createInfo{};
    createInfo.physicalDevice = physicalDevice;
    createInfo.device = logicalDevice;
    createInfo.instance = instance;

    vmaCreateAllocator(&createInfo, &memoryAllocator);
}

void Engine::createMesh(){
    VkResult result;

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = 4 * sizeof(Vertex);
    createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vmaCreateBuffer(memoryAllocator, &createInfo, &allocInfo, &mesh.buffer, &mesh.allocation, nullptr);

    Vertex* vertices = nullptr;
    vmaMapMemory(memoryAllocator, mesh.allocation, reinterpret_cast<void**>(&vertices));

    vertices[0].x = -1;
    vertices[0].y = -1;
    vertices[0].z = 0;
    vertices[0].u = 0;
    vertices[0].v = 0;

    vertices[1].x = +1;
    vertices[1].y = -1;
    vertices[1].z = 0;
    vertices[1].u = 1;
    vertices[1].v = 0;

    vertices[2].x = -1;
    vertices[2].y = +1;
    vertices[2].z = 0;
    vertices[2].u = 0;
    vertices[2].v = 1;

    vertices[3].x = +1;
    vertices[3].y = +1;
    vertices[3].z = 0;
    vertices[3].u = 1;
    vertices[3].v = 1;

    vmaUnmapMemory(memoryAllocator, mesh.allocation);
}

void Engine::createTexture() {
    VkResult result;

    static const int textureWidth = 8;
    static const int textureHeight = 8;

    std::vector<uint32_t> texturePixels(32 * 32, 0xffffffff);

    for (int y = 0; y < textureHeight; y++){
        for (int x = 0; x < textureWidth; x++){
            if ((y % 2 == 0 && x % 2 == 0) || (y % 2 == 1 && x % 2 == 1)){
                texturePixels[y * textureWidth + x] = 0xff0000ff;
            }
        }
    }

    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent = { textureWidth, textureHeight, 1 };
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_LINEAR;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vmaCreateImage(
        memoryAllocator,
        &createInfo,
        &allocInfo,
        &texture.image,
        &texture.allocation,
        nullptr
    );

    VkImageSubresource subresource{};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout layout{};
    vkGetImageSubresourceLayout(logicalDevice, texture.image, &subresource, &layout);

    int cpuPitch = textureWidth;
    int gpuPitch = layout.rowPitch / sizeof(uint32_t);

    uint32_t* cpuPixels = texturePixels.data();
    uint32_t* gpuPixels = nullptr;
    vmaMapMemory(memoryAllocator, texture.allocation, reinterpret_cast<void**>(&gpuPixels));

    for (int y = 0; y < textureHeight; y++){
        for (int x = 0; x < textureWidth; x++){
            gpuPixels[y * gpuPitch + x] = cpuPixels[y * cpuPitch + x];
        }
    }

    vmaUnmapMemory(memoryAllocator, texture.allocation);
}