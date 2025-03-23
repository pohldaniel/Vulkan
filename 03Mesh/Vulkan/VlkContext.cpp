#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <iostream>
#include "Application.h"
#include "VlkExtension.h"
#include "VlkContext.h"
#include "VlkSwapchain.h"
#include "VlkShader.h"

VlkContext vlkContext;

#define ArraySize(arr) sizeof((arr)) / sizeof((arr[0]))

#define VK_CHECK(result)                                      \
    if (result != VK_SUCCESS)                                 \
    {                                                         \
        std::cout << "Vulkan Error: " << result << std::endl; \
        __debugbreak();                                       \
        return false;                                         \
    }

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
	VkDebugUtilsMessageTypeFlagsEXT msgFlags,
	const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
	void* pUserData)
{
	std::cout << pCallbackData->pMessage << std::endl;
	return false;
}

static int MAX_FRAMES_IN_FLIGHT = 1;

bool isTypeOf(VkPhysicalDevice& device, VkPhysicalDeviceType physicalDeviceType) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);
	return properties.deviceType == physicalDeviceType;
}

char* platform_read_file2(const char* path, uint32_t* length) {
    char* result = 0;

    // This opens the file
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (file != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER size;
        if (GetFileSizeEx(file, &size)) {
            *length = size.QuadPart;
            //TODO: Suballocte from main allocation
            result = new char[*length];

            DWORD bytesRead;
            if (ReadFile(file, result, *length, &bytesRead, 0) && bytesRead == *length) {
                // TODO: What to do here?
                // Success
            }
            else {
                //TODO: Assert and error checking
                std::cout << "Failed to read file" << std::endl;
            }
        }
        else {
            //TODO: Assert and error checking
            std::cout << "Failed to get file size" << std::endl;
        }
    }
    else {
        // TODO: Asserts, get error code
        std::cout << "Failed to open the file" << std::endl;
    }

    return result;
}

void vlkInit(void* window) {
    vlkContext.createVkDevice(vlkContext, window);
	
    vkGetDeviceQueue(vlkContext.vkDevice, vlkContext.queueFamilyIndex, 0, &vlkContext.vkQueue);
    vlkContext.createCommandPool();
    vlkContext.createCommandBuffer();

    vlkContext.createDescriptorPool(vlkContext.vkDevice);
    vlkContext.createDescriptorSetLayout(vlkContext.vkDevice);
    vlkContext.createPushConstantRange(vlkContext.vkDevice);
    vlkContext.createPipelineLayout(vlkContext.vkDevice);
    
    vlkContext.createSampler(vlkContext.vkDevice);

    vlkContext.createAllocator();
    vlkContext.createTexture();
    vlkContext.createTextureView(vlkContext.vkDevice);

    vlkContext.createShaders(vlkContext.vkDevice); 
    vlkContext.createMesh();
    vlkContext.resize();
    //vlkContext.createUniformBuffers(vlkContext.vkDevice);

   
    vlkContext.swapchain = new VlkSwapchain(&vlkContext, Application::Width, Application::Height, vlkContext.vkPresentModeKHR);
   
}

void vlkResize() {
    const VkDevice& vkDevice = vlkContext.vkDevice;
    vkDeviceWaitIdle(vkDevice);

    vlkContext.newSwapchain = new VlkSwapchain(&vlkContext, Application::Width, Application::Height, vlkContext.vkPresentModeKHR, vlkContext.swapchain->swapchain);
    delete vlkContext.swapchain;
    vlkContext.swapchain = vlkContext.newSwapchain;
}

void vlkToggleVerticalSync() {
    if (vlkContext.vkPresentModeKHR == VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR)
        vlkContext.vkPresentModeKHR = VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;
    else
        vlkContext.vkPresentModeKHR = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;

    const VkDevice& vkDevice = vlkContext.vkDevice;
    vkDeviceWaitIdle(vkDevice);

    vlkContext.newSwapchain = new VlkSwapchain(&vlkContext, Application::Width, Application::Height, vlkContext.vkPresentModeKHR, vlkContext.swapchain->swapchain);
    delete vlkContext.swapchain;
    vlkContext.swapchain = vlkContext.newSwapchain;
}

void vlkToggleWireframe() {
    if (vlkContext.vkPolygonMode == VkPolygonMode::VK_POLYGON_MODE_FILL)
        vlkContext.vkPolygonMode = VkPolygonMode::VK_POLYGON_MODE_LINE;
    else
        vlkContext.vkPolygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
}

void vlkDraw(const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount) {
    bool shouldResize = vlkContext.swapchain->draw(vlkContext.ubo, vertex, index, drawCount);

    if (shouldResize){
        vlkContext.resize();
    }
}

void vlkMapBuffer(const VkDeviceMemory& vkDeviceMemory, const void* data, uint32_t size) {
    const VkDevice& vkDevice = vlkContext.vkDevice;

    void* pMem = nullptr;
    vkMapMemory(vkDevice, vkDeviceMemory, 0, size, 0, &pMem);
    memcpy(pMem, data, size);
    vkUnmapMemory(vkDevice, vkDeviceMemory);
}

void vlkCreateBuffer(VkBuffer& vkBuffer, VkDeviceMemory& vkDeviceMemory, uint32_t size, VkBufferUsageFlags vkBufferUsageFlags, VkMemoryPropertyFlags vkMemoryPropertyFlags) {
    const VkDevice& vkDevice = vlkContext.vkDevice;
    
    VkBufferCreateInfo vkBufferCreateInfo = {};
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.size = size;
    vkBufferCreateInfo.usage = vkBufferUsageFlags;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vkDevice, &vkBufferCreateInfo, NULL, &vkBuffer);

    VkMemoryRequirements vkMemoryRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vkBuffer, &vkMemoryRequirements);

    VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = vlkContext.GetMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, vkMemoryPropertyFlags);

    vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, NULL, &vkDeviceMemory);
    vkBindBufferMemory(vkDevice, vkBuffer, vkDeviceMemory, 0);
}

void vlkCopyBuffer(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, uint32_t size) {
    const VkCommandBuffer& vkCommandBuffer = vlkContext.vkCommandBuffer;

    VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
    vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkCommandBufferBeginInfo.pNext = NULL;
    vkCommandBufferBeginInfo.pInheritanceInfo = NULL;

    vkBeginCommandBuffer(vkCommandBuffer, &vkCommandBufferBeginInfo);

    VkBufferCopy vkBufferCopy = {};
    vkBufferCopy.srcOffset = 0;
    vkBufferCopy.dstOffset = 0;
    vkBufferCopy.size = size;

    vkCmdCopyBuffer(vkCommandBuffer, srcBuffer, dstBuffer, 1, &vkBufferCopy);
    vkEndCommandBuffer(vkCommandBuffer);

    vlkQueueSubmit(vkCommandBuffer);
}

void vlkCreateImage(VkImage& vkImage, VkDeviceMemory& vkDeviceMemory, uint32_t width, uint32_t height, VkFormat vkFormat, VkImageTiling vkImageTiling, VkImageUsageFlags vkImageUsageFlags, VkMemoryPropertyFlags vkMemoryPropertyFlags) {
    const VkDevice& vkDevice = vlkContext.vkDevice;
    
    VkImageCreateInfo vkImageCreateInfo = {};
    vkImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    vkImageCreateInfo.extent.width = width;
    vkImageCreateInfo.extent.height = height;
    vkImageCreateInfo.extent.depth = 1;
    vkImageCreateInfo.mipLevels = 1;
    vkImageCreateInfo.arrayLayers = 1;
    vkImageCreateInfo.format = vkFormat;
    vkImageCreateInfo.tiling = vkImageTiling;
    vkImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkImageCreateInfo.usage = vkImageUsageFlags;
    vkImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateImage(vkDevice, &vkImageCreateInfo, nullptr, &vkImage);

    VkMemoryRequirements vkMemoryRequirements;
    vkGetImageMemoryRequirements(vkDevice, vkImage, &vkMemoryRequirements);

    VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
    vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
    vkMemoryAllocateInfo.memoryTypeIndex = vlkContext.findMemoryType(vkMemoryRequirements.memoryTypeBits, vkMemoryPropertyFlags);

    vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, nullptr, &vkDeviceMemory);
    vkBindImageMemory(vkDevice, vkImage, vkDeviceMemory, 0);
}

void vlkCreateImageView(VkImageView& vkImageView, const VkImage& vkImage, VkFormat vkFormat, VkImageAspectFlags vkImageAspectFlags, VkComponentMapping vkComponentMapping) {
    const VkDevice& vkDevice = vlkContext.vkDevice;
    
    VkImageViewCreateInfo vkImageViewCreateInfo = {};
    vkImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vkImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vkImageViewCreateInfo.image = vkImage;
    vkImageViewCreateInfo.format = vkFormat;
    vkImageViewCreateInfo.subresourceRange.aspectMask = vkImageAspectFlags;
    vkImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    vkImageViewCreateInfo.subresourceRange.levelCount = 1;
    vkImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    vkImageViewCreateInfo.subresourceRange.layerCount = 1;
    vkImageViewCreateInfo.components = vkComponentMapping;
    vkCreateImageView(vkDevice, &vkImageViewCreateInfo, NULL, &vkImageView);
}

void vlkDestroyImage(const VkImage& vkImage, const VkDeviceMemory& vkDeviceMemory) {
    const VkDevice& vkDevice = vlkContext.vkDevice;
    if(vkDeviceMemory)
      vkFreeMemory(vkDevice, vkDeviceMemory, NULL);
    vkDestroyImage(vkDevice, vkImage, NULL);
}

void vlkDestroyImage(const VkImage& vkImage) {
    const VkDevice& vkDevice = vlkContext.vkDevice;
    vkDestroyImage(vkDevice, vkImage, NULL);
}

void vlkCreateCommandBuffer(VkCommandBuffer& vkCommandBuffer) {
    const VkDevice& vkDevice = vlkContext.vkDevice;
    VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo{};
    vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vkCommandBufferAllocateInfo.commandPool = vlkContext.vkCommandPool;
    vkCommandBufferAllocateInfo.commandBufferCount = 1;
    vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(vkDevice, &vkCommandBufferAllocateInfo, &vkCommandBuffer);
}

void vlkTransitionImageLayout(const VkCommandBuffer& commandBuffer, const VkImage& vkImage, VkImageAspectFlags vkImageAspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
    VkImageSubresourceRange access;
    access.aspectMask = vkImageAspectFlags;
    access.baseMipLevel = 0;
    access.levelCount = 1;
    access.baseArrayLayer = 0;
    access.layerCount = 1;

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vkImage;
    barrier.subresourceRange = access;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, 0, 0, 0, 1, &barrier);
}

void vlkCreateSemaphore(VkSemaphore& vkSemaphore) {
    const VkDevice& vkDevice = vlkContext.vkDevice;

    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(vkDevice, &createInfo, nullptr, &vkSemaphore);
}

void vlkCreateFence(VkFence& vkFence) {
    const VkDevice& vkDevice = vlkContext.vkDevice;

    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(vkDevice, &createInfo, nullptr, &vkFence);
}

void vlkBeginCommandBuffer(const VkCommandBuffer& vkCommandBuffer, VkCommandBufferUsageFlags vkCommandBufferUsageFlags) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = vkCommandBufferUsageFlags;
    vkBeginCommandBuffer(vkCommandBuffer, &beginInfo);
}

void vlkEndCommandBuffer(const VkCommandBuffer& vkCommandBuffer) {
    vkEndCommandBuffer(vkCommandBuffer);
}

void vlkQueueSubmit(const VkCommandBuffer& vkCommandBuffer) {
    const VkQueue& vkQueue = vlkContext.vkQueue;

    VkSubmitInfo vkSubmitInfo = {};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSubmitInfo.commandBufferCount = 1;
    vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
    vkSubmitInfo.waitSemaphoreCount = 0;
    vkSubmitInfo.pWaitSemaphores = VK_NULL_HANDLE;
    vkSubmitInfo.pWaitDstStageMask = VK_NULL_HANDLE;
    vkSubmitInfo.signalSemaphoreCount = 0;
    vkSubmitInfo.pSignalSemaphores = VK_NULL_HANDLE;
    vkSubmitInfo.pNext = NULL;

    vkQueueSubmit(vkQueue, 1, &vkSubmitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkQueue);
}

void vlkCreateSwapChain(VkSwapchainKHR& vkSwapchainKHR, VkFormat& vkFormat, uint32_t width, uint32_t height, const VkPresentModeKHR vkPresentModeKHR, VkSwapchainKHR vkOldSwapchainKHR) {
    const VkDevice& vkDevice = vlkContext.vkDevice;
    const VkPhysicalDevice& vkPhysicalDevice = vlkContext.vkPhysicalDevice;
    const VkSurfaceKHR& vkSurfaceKHR = vlkContext.vkSurfaceKHR;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, vkSurfaceKHR, &capabilities);

    width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurfaceKHR, &formatCount, VK_NULL_HANDLE);

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurfaceKHR, &formatCount, formats.data());

    VkSurfaceFormatKHR chosenFormat = formats[0];

    for (const VkSurfaceFormatKHR& format : formats){
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM){
            chosenFormat = format;
            break;
        }
    }

    vkFormat = chosenFormat.format;
    
    VkSwapchainCreateInfoKHR vkSwapchainCreateInfoKHR = {};
    vkSwapchainCreateInfoKHR.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vkSwapchainCreateInfoKHR.surface = vkSurfaceKHR;
    vkSwapchainCreateInfoKHR.minImageCount = capabilities.minImageCount;
    vkSwapchainCreateInfoKHR.imageFormat = chosenFormat.format;
    vkSwapchainCreateInfoKHR.imageColorSpace = chosenFormat.colorSpace;
    vkSwapchainCreateInfoKHR.imageExtent = { width, height };
    vkSwapchainCreateInfoKHR.imageArrayLayers = 1;
    vkSwapchainCreateInfoKHR.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vkSwapchainCreateInfoKHR.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkSwapchainCreateInfoKHR.preTransform = capabilities.currentTransform;
    vkSwapchainCreateInfoKHR.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vkSwapchainCreateInfoKHR.clipped = true;
    vkSwapchainCreateInfoKHR.presentMode = vkPresentModeKHR;
    //createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    vkSwapchainCreateInfoKHR.oldSwapchain = vkOldSwapchainKHR;
    vkCreateSwapchainKHR(vkDevice, &vkSwapchainCreateInfoKHR, VK_NULL_HANDLE, &vkSwapchainKHR);
}

bool VlkContext::createVkDevice(VlkContext& vlkContext, void* window){
	/*uint32_t amountOfInstanceLayers = 0;
	vkEnumerateInstanceLayerProperties(&amountOfInstanceLayers, NULL);
	VkLayerProperties* instanceLayers = new VkLayerProperties[amountOfInstanceLayers];
	vkEnumerateInstanceLayerProperties(&amountOfInstanceLayers, instanceLayers);

	std::cout << "Amount of Instance Layers: " << amountOfInstanceLayers << std::endl;

	for (size_t i = 0; i < amountOfInstanceLayers; i++){
		std::cout << std::endl;
		std::cout << "Name:         " << instanceLayers[i].layerName << std::endl;
		std::cout << "Spec Version: " << instanceLayers[i].specVersion << std::endl;
		std::cout << "Impl Version: " << instanceLayers[i].implementationVersion << std::endl;
		std::cout << "Description:  " << instanceLayers[i].description << std::endl;
	}

	uint32_t amountOfInstanceExtensions = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &amountOfInstanceExtensions, NULL);
	VkExtensionProperties* instanceExtensions = new VkExtensionProperties[amountOfInstanceExtensions];
	vkEnumerateInstanceExtensionProperties(NULL, &amountOfInstanceExtensions, instanceExtensions);
	std::cout << std::endl;
	std::cout << "Amount of Instance Extension: " << amountOfInstanceExtensions << std::endl;

	for (size_t i = 0; i < amountOfInstanceExtensions; i++) {
		std::cout << std::endl;
		std::cout << "Name:         " << instanceExtensions[i].extensionName << std::endl;
		std::cout << "Spec Version: " << instanceExtensions[i].specVersion << std::endl;
	}*/

	vlkContext.screenSize.width = Application::Width;
	vlkContext.screenSize.height = Application::Height;

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "vkApplication";
	appInfo.pEngineName = "vkEngine";
	appInfo.apiVersion = VK_API_VERSION_1_4;

	const char* instanceExtensionsList[] = {
		"VK_KHR_win32_surface",
		"VK_EXT_debug_utils",
		"VK_KHR_surface",
	};

	const char* instanceLayersList[]{
		"VK_LAYER_KHRONOS_validation",
	};

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.ppEnabledExtensionNames = instanceExtensionsList;
	instanceInfo.enabledExtensionCount = ArraySize(instanceExtensionsList);
	instanceInfo.ppEnabledLayerNames = instanceLayersList;
	instanceInfo.enabledLayerCount = ArraySize(instanceLayersList);
	VK_CHECK(vkCreateInstance(&instanceInfo, 0, &vlkContext.vkInstance));

    vkExtensionInit(vlkContext.vkInstance);

	VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debugInfo.pfnUserCallback = vk_debug_callback;

	vkCreateDebugUtilsMessengerEXT(vlkContext.vkInstance, &debugInfo, 0, &vkDebugMessenger);

	VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.hwnd = (HWND)window;
	surfaceInfo.hinstance = GetModuleHandleA(0);
	VK_CHECK(vkCreateWin32SurfaceKHR(vlkContext.vkInstance, &surfaceInfo, 0, &vlkContext.vkSurfaceKHR));

	vlkContext.queueFamilyIndex = 0;

	uint32_t amountOfPhysicalDevices = 0;
	//TODO: Suballocation from Main Allocation
	//VkPhysicalDevice gpus[10];
	VK_CHECK(vkEnumeratePhysicalDevices(vlkContext.vkInstance, &amountOfPhysicalDevices, 0));
	VkPhysicalDevice* vkPhysicalDevices = new VkPhysicalDevice[amountOfPhysicalDevices];
	VK_CHECK(vkEnumeratePhysicalDevices(vlkContext.vkInstance, &amountOfPhysicalDevices, vkPhysicalDevices));

	for (uint32_t i = 0; i < amountOfPhysicalDevices; i++) {
		VkPhysicalDevice vkPhysicalDevice = vkPhysicalDevices[i];
		//printStats(vkPhysicalDevice);
		if (isTypeOf(vkPhysicalDevice, VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU))
			continue;

		uint32_t amountOfqueueFamilies = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &amountOfqueueFamilies, 0);
		VkQueueFamilyProperties* vkQueueFamilyProperties = new VkQueueFamilyProperties[amountOfqueueFamilies];
		vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &amountOfqueueFamilies, vkQueueFamilyProperties);

		for (uint32_t j = 0; j < amountOfqueueFamilies; j++) {
			if (vkQueueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				VkBool32 surfaceSupport = VK_FALSE;
				VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, j, vlkContext.vkSurfaceKHR, &surfaceSupport));

				if (surfaceSupport) {
					vlkContext.queueFamilyIndex = j;
					vlkContext.vkPhysicalDevice = vkPhysicalDevice;
					break;
				}
			}
		}
		delete[] vkQueueFamilyProperties;
	}
	delete[] vkPhysicalDevices;

	/*uint32_t amountOfDeviceLayers = 0;
	vkEnumerateDeviceLayerProperties(vlkcontext->gpu, &amountOfDeviceLayers, NULL);
	VkLayerProperties* deviceLayers = new VkLayerProperties[amountOfDeviceLayers];
	vkEnumerateDeviceLayerProperties(vlkcontext->gpu, &amountOfDeviceLayers, deviceLayers);
	std::cout << std::endl;
	std::cout << "Amount of Device Layers: " << amountOfDeviceLayers << std::endl;

	for (size_t i = 0; i < amountOfDeviceLayers; i++) {
		std::cout << std::endl;
		std::cout << "Name:         " << deviceLayers[i].layerName << std::endl;
		std::cout << "Spec Version: " << deviceLayers[i].specVersion << std::endl;
		std::cout << "Impl Version: " << deviceLayers[i].implementationVersion << std::endl;
		std::cout << "Description:  " << deviceLayers[i].description << std::endl;
	}

	uint32_t amountOfDeviceExtensions = 0;
	vkEnumerateDeviceExtensionProperties(vlkcontext->gpu, NULL, &amountOfDeviceExtensions, NULL);
	VkExtensionProperties* deviceExtensions = new VkExtensionProperties[amountOfDeviceExtensions];
	vkEnumerateDeviceExtensionProperties(vlkcontext->gpu, NULL, &amountOfDeviceExtensions, deviceExtensions);
	std::cout << std::endl;
	std::cout << "Amount of Device Extension: " << amountOfDeviceExtensions << std::endl;
	std::cout << std::endl;
	for (size_t i = 0; i < amountOfDeviceExtensions; i++) {

		std::cout << "Name:         " << deviceExtensions[i].extensionName << std::endl;
		//std::cout << "Spec Version: " << deviceExtensions[i].specVersion << std::endl;
	}*/

	const char* deviceExtensionsList[] = {
		"VK_KHR_shader_terminate_invocation",
		"VK_KHR_dynamic_rendering",
		"VK_EXT_shader_object",
		"VK_KHR_swapchain"
	};

	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueInfo = {};
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueFamilyIndex = vlkContext.queueFamilyIndex;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
	deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
    deviceFeatures.wideLines = VK_TRUE;

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering = {};
	dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamicRendering.dynamicRendering = true;

	VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.features = deviceFeatures;
	deviceFeatures2.pNext = &dynamicRendering;

	VkPhysicalDeviceShaderObjectFeaturesEXT deviceShaderObjectFeatures = {};
	deviceShaderObjectFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
	deviceShaderObjectFeatures.shaderObject = VK_TRUE;
	deviceShaderObjectFeatures.pNext = &deviceFeatures2;

	VkPhysicalDeviceSynchronization2Features deviceSynchronization2Features = {};
	deviceSynchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	deviceSynchronization2Features.synchronization2 = VK_TRUE;
	deviceSynchronization2Features.pNext = &deviceShaderObjectFeatures;

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures = {};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	indexingFeatures.pNext = &deviceSynchronization2Features;
	indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	indexingFeatures.runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT unusedAttachmentsFeatures = {};
    unusedAttachmentsFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT;
    unusedAttachmentsFeatures.dynamicRenderingUnusedAttachments = VK_TRUE;
    unusedAttachmentsFeatures.pNext = &indexingFeatures;

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.ppEnabledExtensionNames = deviceExtensionsList;
	deviceInfo.enabledExtensionCount = ArraySize(deviceExtensionsList);
	//deviceInfo.pEnabledFeatures = &deviceFeatures;
	deviceInfo.pEnabledFeatures = NULL;
	deviceInfo.pNext = &unusedAttachmentsFeatures;

	vkCreateDevice(vlkContext.vkPhysicalDevice, &deviceInfo, 0, &vlkContext.vkDevice);

    const VkFormat depthFormats[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };


    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkFormatFeatureFlagBits features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (uint32_t i = 0; i < 3; i++) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(vlkContext.vkPhysicalDevice, depthFormats[i], &properties);
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
            vkDepthFormat = depthFormats[i];
        }else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
            vkDepthFormat = depthFormats[i];
        }
    }
    
	return true;
}

void VlkContext::resize(){
	/*VkResult result;

	vkDeviceWaitIdle(logicalDevice);

	delete swapchain;
	swapchain = new Swapchain(this, width, height);*/
}

void VlkContext::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    vkCreateCommandPool(vkDevice, &poolInfo, 0, &vkCommandPool);
}

void VlkContext::createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vkCommandPool;
    allocInfo.commandBufferCount = 1;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(vkDevice, &allocInfo, &vkCommandBuffer);
}

void VlkContext::createBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    VkBufferCreateInfo vkBufferCreateInfo = {};
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.size = size;
    vkBufferCreateInfo.usage = usage;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vlkContext.vkDevice, &vkBufferCreateInfo, NULL, &buffer);

    VkMemoryRequirements vkMemoryRequirements;
    vkGetBufferMemoryRequirements(vlkContext.vkDevice, buffer, &vkMemoryRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = vkMemoryRequirements.size;
    allocInfo.memoryTypeIndex = vlkContext.GetMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, properties);
    
    vkAllocateMemory(vlkContext.vkDevice, &allocInfo, NULL, &bufferMemory);
    vkBindBufferMemory(vlkContext.vkDevice, buffer, bufferMemory, 0);
}

void VlkContext::mapBuffer(const VkDeviceMemory& bufferMemory, const void* data, uint32_t size) {
    void* pMem = nullptr;
    vkMapMemory(vlkContext.vkDevice, bufferMemory, 0, size, 0, &pMem);
    memcpy(pMem, data, size);
    vkUnmapMemory(vlkContext.vkDevice, bufferMemory);
}

void VlkContext::copyBuffer(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, uint32_t size) {
    VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
    vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkCommandBufferBeginInfo.pNext = NULL;
    vkCommandBufferBeginInfo.pInheritanceInfo = NULL;

    vkBeginCommandBuffer(vkCommandBuffer, &vkCommandBufferBeginInfo);

    VkBufferCopy vkBufferCopy = {};
    vkBufferCopy.srcOffset = 0;
    vkBufferCopy.dstOffset = 0;
    vkBufferCopy.size = size;

    vkCmdCopyBuffer(vkCommandBuffer, srcBuffer, dstBuffer, 1, &vkBufferCopy);
    vkEndCommandBuffer(vkCommandBuffer);
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCommandBuffer;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = VK_NULL_HANDLE;
    submitInfo.pWaitDstStageMask = VK_NULL_HANDLE;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = VK_NULL_HANDLE;
    submitInfo.pNext = NULL;

    vkQueueSubmit(vlkContext.vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vlkContext.vkQueue);
}

void VlkContext::createAllocator() {
    VkResult result;

    VmaAllocatorCreateInfo createInfo{};
    createInfo.physicalDevice = vkPhysicalDevice;
    createInfo.device = vkDevice;
    createInfo.instance = vkInstance;

    vmaCreateAllocator(&createInfo, &memoryAllocator);
}

void VlkContext::createMesh() {
    VkResult result;

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = 4 * sizeof(Vertex);
    createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vmaCreateBuffer(memoryAllocator, &createInfo, &allocInfo, &vmaBuffer.buffer, &vmaBuffer.allocation, nullptr);

    Vertex* vertices = nullptr;
    vmaMapMemory(memoryAllocator, vmaBuffer.allocation, reinterpret_cast<void**>(&vertices));

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

    vmaUnmapMemory(memoryAllocator, vmaBuffer.allocation);
}

void VlkContext::createTexture() {
    VkResult result;

    static const int textureWidth = 8;
    static const int textureHeight = 8;

    std::vector<uint32_t> texturePixels(32 * 32, 0xffffffff);

    for (int y = 0; y < textureHeight; y++) {
        for (int x = 0; x < textureWidth; x++) {
            if ((y % 2 == 0 && x % 2 == 0) || (y % 2 == 1 && x % 2 == 1)) {
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
        &vmaImage.image,
        &vmaImage.allocation,
        nullptr
    );

    VkImageSubresource subresource{};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout layout{};
    vkGetImageSubresourceLayout(vkDevice, vmaImage.image, &subresource, &layout);

    int cpuPitch = textureWidth;
    int gpuPitch = layout.rowPitch / sizeof(uint32_t);

    uint32_t* cpuPixels = texturePixels.data();
    uint32_t* gpuPixels = nullptr;
    vmaMapMemory(memoryAllocator, vmaImage.allocation, reinterpret_cast<void**>(&gpuPixels));

    for (int y = 0; y < textureHeight; y++) {
        for (int x = 0; x < textureWidth; x++) {
            gpuPixels[y * gpuPitch + x] = cpuPixels[y * cpuPitch + x];
        }
    }

    vmaUnmapMemory(memoryAllocator, vmaImage.allocation);
}

void VlkContext::createDescriptorPool(const VkDevice& vkDevice) {
    VkResult result;

    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxDescriptorSets * maxDescriptorCount },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxDescriptorSets * maxDescriptorCount },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxDescriptorSets * maxDescriptorCount }
    };
    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.maxSets = maxDescriptorSets;
    createInfo.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
    createInfo.pPoolSizes = poolSizes;

    vkCreateDescriptorPool(vkDevice, &createInfo, nullptr, &descriptorPool);
}

void VlkContext::createDescriptorSetLayout(const VkDevice& vkDevice) {
    VkResult result;

    VkDescriptorSetLayoutBinding bindings[3]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = maxDescriptorCount;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = maxDescriptorCount;
    bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount = maxDescriptorCount;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorBindingFlags bindingFlags[] = {
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{};
    bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsCreateInfo.pBindingFlags = bindingFlags;
    bindingFlagsCreateInfo.bindingCount = sizeof(bindingFlags) / sizeof(VkDescriptorBindingFlags);

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = &bindingFlagsCreateInfo;
    createInfo.bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding);
    createInfo.pBindings = bindings;

    vkCreateDescriptorSetLayout(vkDevice, &createInfo, nullptr, &descriptorSetLayout);
}

void VlkContext::createPushConstantRange(const VkDevice& vkDevice) {
    pushConstantRange = VkPushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.size = pushConstantRangeSize;
}

void VlkContext::createPipelineLayout(const VkDevice& vkDevice) {
    VkResult result;

    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &descriptorSetLayout;
    createInfo.pushConstantRangeCount = 1;
    createInfo.pPushConstantRanges = &pushConstantRange;

    vkCreatePipelineLayout(vkDevice, &createInfo, nullptr, &pipelineLayout);
}

void VlkContext::createSampler(const VkDevice& vkDevice) {
    VkResult result;

    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.maxAnisotropy = 16;
    createInfo.maxLod = VK_LOD_CLAMP_NONE;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    vkCreateSampler(vkDevice, &createInfo, nullptr, &sampler);
}

void VlkContext::createShaders(const VkDevice& vkDevice){
  

    std::vector<uint32_t> vertexCode, fragmentCode;
    VlkShader shader;
    std::vector<VkShaderEXT> _shaders = shader.make_shader_objects(vkInstance, vkDevice, "mesh", vertexCode, fragmentCode, descriptorSetLayout, pushConstantRange, true);
    shaders[0] = _shaders[0];
    shaders[1] = _shaders[1];
   
   
}

void VlkContext::createTextureView(const VkDevice& vkDevice){
    VkResult result;

    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    createInfo.image = vmaImage.image;
    createInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCreateImageView(vkDevice, &createInfo, nullptr, &textureView);
}


void VlkContext::createUniformBuffers(const VkDevice& vkDevice) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    vkBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    vkDeviceMemory.resize(MAX_FRAMES_IN_FLIGHT);
    vkBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(vkDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vkBuffers[i], vkDeviceMemory[i]);
        vkMapMemory(vkDevice, vkDeviceMemory[i], 0, bufferSize, 0, &vkBuffersMapped[i]);
    }
}

void VlkContext::createBuffer(const VkDevice& vkDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(vkDevice, buffer, bufferMemory, 0);
}

uint32_t VlkContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

void VlkContext::updateUniformBuffer(const UniformBufferObject& ubo) {
    memcpy(vkBuffersMapped[0], &ubo, sizeof(ubo));
}

uint32_t VlkContext::GetMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) const{
    VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkPhysicalDeviceMemoryProperties);

    for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        const VkMemoryType& memType = vkPhysicalDeviceMemoryProperties.memoryTypes[i];
        uint32_t bitmask = (1 << i);
        bool isMemTypeSupported = (typeFilter & bitmask);
        bool hasRequiredMemProps = ((memType.propertyFlags & properties) == properties);

        if (isMemTypeSupported && hasRequiredMemProps)
            return i;

        //if ((typeFilter & (1 << i)) && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
           // return i;
        //}
    }
    std::cout << "Failed: " << std::endl;
    return 0;
}