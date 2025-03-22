#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <iostream>
#include "VkExtension.h"
#include "Application.h"
#include "VkContext.h"
#include "swap_chain.h"

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

void vlkInit(VkContext& vkContext, void* window) {
    vkContext.createVkDevice(vkContext, window);
	
    vkGetDeviceQueue(vkContext.vkDevice, vkContext.queueFamilyIndex, 0, &vkContext.vkQueue);
    vkContext.createCommandPool();
    vkContext.createCommandBuffer();

    vkContext.createDescriptorPool(vkContext.vkDevice);
    vkContext.createDescriptorSetLayout(vkContext.vkDevice);
    vkContext.createPushConstantRange(vkContext.vkDevice);
    vkContext.createPipelineLayout(vkContext.vkDevice);
    
    vkContext.createSampler(vkContext.vkDevice);

    vkContext.createAllocator();
    vkContext.createTexture();
    vkContext.createTextureView(vkContext.vkDevice);

    vkContext.createShaders(vkContext.vkDevice); 
    vkContext.createMesh();
    vkContext.resize();
    //vkContext.createUniformBuffers(vkContext.vkDevice);
    vkContext.swapchain = new Swapchain(&vkContext, Application::Width, Application::Height);
    //vkContext.createPipeline(vkContext.vkDevice);
}

void vlkDraw(VkContext& vkContext, const VkBuffer& vertex, const VkBuffer& index, const uint32_t drawCount) {
    bool shouldResize = vkContext.swapchain->draw(vkContext.ubo, vertex, index, drawCount);

    if (shouldResize){
        vkContext.resize();
    }
}

void vlkMapBuffer(const VkDeviceMemory& bufferMemory, const void* data, uint32_t size) {
    const VkDevice& vkDevice = Application::VkContext.vkDevice;

    void* pMem = nullptr;
    vkMapMemory(vkDevice, bufferMemory, 0, size, 0, &pMem);
    memcpy(pMem, data, size);
    vkUnmapMemory(vkDevice, bufferMemory);
}

void vlkCreateBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    const VkDevice& vkDevice = Application::VkContext.vkDevice;
    
    VkBufferCreateInfo vkBufferCreateInfo = {};
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.size = size;
    vkBufferCreateInfo.usage = usage;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vkDevice, &vkBufferCreateInfo, NULL, &buffer);

    VkMemoryRequirements vkMemoryRequirements;
    vkGetBufferMemoryRequirements(vkDevice, buffer, &vkMemoryRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = vkMemoryRequirements.size;
    allocInfo.memoryTypeIndex = Application::VkContext.GetMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, properties);

    vkAllocateMemory(vkDevice, &allocInfo, NULL, &bufferMemory);
    vkBindBufferMemory(vkDevice, buffer, bufferMemory, 0);
}

void vlkCopyBuffer(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, uint32_t size) {
    const VkCommandBuffer& vkCommandBuffer = Application::VkContext.vkCommandBuffer;

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

    vkQueueSubmit(Application::VkContext.vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(Application::VkContext.vkQueue);
}

void vlkToggleWireframe() {
    if (Application::VkContext.vkPolygonMode == VkPolygonMode::VK_POLYGON_MODE_FILL)
        Application::VkContext.vkPolygonMode = VkPolygonMode::VK_POLYGON_MODE_LINE;
    else
        Application::VkContext.vkPolygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
}

bool VkContext::createVkDevice(VkContext& vkContext, void* window){
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

	vkContext.screenSize.width = Application::Width;
	vkContext.screenSize.height = Application::Height;

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "vkApplication";
	appInfo.pEngineName = "vkEngine";
	appInfo.apiVersion = VK_API_VERSION_1_3;

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
	VK_CHECK(vkCreateInstance(&instanceInfo, 0, &vkContext.vkInstance));

    vkExtensionInit(vkContext.vkInstance);

	VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debugInfo.pfnUserCallback = vk_debug_callback;

	vkCreateDebugUtilsMessengerEXT(vkContext.vkInstance, &debugInfo, 0, &vkDebugMessenger);

	VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.hwnd = (HWND)window;
	surfaceInfo.hinstance = GetModuleHandleA(0);
	VK_CHECK(vkCreateWin32SurfaceKHR(vkContext.vkInstance, &surfaceInfo, 0, &vkContext.vkSurfaceKHR));

	vkContext.queueFamilyIndex = 0;

	uint32_t amountOfPhysicalDevices = 0;
	//TODO: Suballocation from Main Allocation
	//VkPhysicalDevice gpus[10];
	VK_CHECK(vkEnumeratePhysicalDevices(vkContext.vkInstance, &amountOfPhysicalDevices, 0));
	VkPhysicalDevice* vkPhysicalDevices = new VkPhysicalDevice[amountOfPhysicalDevices];
	VK_CHECK(vkEnumeratePhysicalDevices(vkContext.vkInstance, &amountOfPhysicalDevices, vkPhysicalDevices));

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
				VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, j, vkContext.vkSurfaceKHR, &surfaceSupport));

				if (surfaceSupport) {
					vkContext.queueFamilyIndex = j;
					vkContext.vkPhysicalDevice = vkPhysicalDevice;
					break;
				}
			}
		}
		delete[] vkQueueFamilyProperties;
	}
	delete[] vkPhysicalDevices;

	/*uint32_t amountOfDeviceLayers = 0;
	vkEnumerateDeviceLayerProperties(vkcontext->gpu, &amountOfDeviceLayers, NULL);
	VkLayerProperties* deviceLayers = new VkLayerProperties[amountOfDeviceLayers];
	vkEnumerateDeviceLayerProperties(vkcontext->gpu, &amountOfDeviceLayers, deviceLayers);
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
	vkEnumerateDeviceExtensionProperties(vkcontext->gpu, NULL, &amountOfDeviceExtensions, NULL);
	VkExtensionProperties* deviceExtensions = new VkExtensionProperties[amountOfDeviceExtensions];
	vkEnumerateDeviceExtensionProperties(vkcontext->gpu, NULL, &amountOfDeviceExtensions, deviceExtensions);
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
	queueInfo.queueFamilyIndex = vkContext.queueFamilyIndex;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
	deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;


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

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.ppEnabledExtensionNames = deviceExtensionsList;
	deviceInfo.enabledExtensionCount = ArraySize(deviceExtensionsList);
	//deviceInfo.pEnabledFeatures = &deviceFeatures;
	deviceInfo.pEnabledFeatures = NULL;
	deviceInfo.pNext = &indexingFeatures;

	vkCreateDevice(vkContext.vkPhysicalDevice, &deviceInfo, 0, &vkContext.vkDevice);

    const VkFormat depthFormats[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };


    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkFormatFeatureFlagBits features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (uint32_t i = 0; i < 3; i++) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(vkContext.vkPhysicalDevice, depthFormats[i], &properties);
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
            vkDepthFormat = depthFormats[i];
        }else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
            vkDepthFormat = depthFormats[i];
        }
    }
    
	return true;
}

void VkContext::resize(){
	/*VkResult result;

	vkDeviceWaitIdle(logicalDevice);

	delete swapchain;
	swapchain = new Swapchain(this, width, height);*/
}

void VkContext::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    vkCreateCommandPool(vkDevice, &poolInfo, 0, &vkCommandPool);
}

void VkContext::createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vkCommandPool;
    allocInfo.commandBufferCount = 1;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(vkDevice, &allocInfo, &vkCommandBuffer);
}

void VkContext::createBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    VkBufferCreateInfo vkBufferCreateInfo = {};
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.size = size;
    vkBufferCreateInfo.usage = usage;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(Application::VkContext.vkDevice, &vkBufferCreateInfo, NULL, &buffer);

    VkMemoryRequirements vkMemoryRequirements;
    vkGetBufferMemoryRequirements(Application::VkContext.vkDevice, buffer, &vkMemoryRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = vkMemoryRequirements.size;
    allocInfo.memoryTypeIndex = Application::VkContext.GetMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, properties);
    
    vkAllocateMemory(Application::VkContext.vkDevice, &allocInfo, NULL, &bufferMemory);
    vkBindBufferMemory(Application::VkContext.vkDevice, buffer, bufferMemory, 0);
}

void VkContext::mapBuffer(const VkDeviceMemory& bufferMemory, const void* data, uint32_t size) {
    void* pMem = nullptr;
    vkMapMemory(Application::VkContext.vkDevice, bufferMemory, 0, size, 0, &pMem);
    memcpy(pMem, data, size);
    vkUnmapMemory(Application::VkContext.vkDevice, bufferMemory);
}

void VkContext::copyBuffer(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, uint32_t size) {
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

    vkQueueSubmit(Application::VkContext.vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(Application::VkContext.vkQueue);
}

void VkContext::createAllocator() {
    VkResult result;

    VmaAllocatorCreateInfo createInfo{};
    createInfo.physicalDevice = vkPhysicalDevice;
    createInfo.device = vkDevice;
    createInfo.instance = vkInstance;

    vmaCreateAllocator(&createInfo, &memoryAllocator);
}

void VkContext::createMesh() {
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

void VkContext::createTexture() {
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

void VkContext::createDescriptorPool(const VkDevice& vkDevice) {
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

void VkContext::createDescriptorSetLayout(const VkDevice& vkDevice) {
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

void VkContext::createPushConstantRange(const VkDevice& vkDevice) {
    pushConstantRange = VkPushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.size = pushConstantRangeSize;
}

void VkContext::createPipelineLayout(const VkDevice& vkDevice) {
    VkResult result;

    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &descriptorSetLayout;
    createInfo.pushConstantRangeCount = 1;
    createInfo.pPushConstantRanges = &pushConstantRange;

    vkCreatePipelineLayout(vkDevice, &createInfo, nullptr, &pipelineLayout);
}

void VkContext::createSampler(const VkDevice& vkDevice) {
    VkResult result;

    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.maxAnisotropy = 16;
    createInfo.maxLod = VK_LOD_CLAMP_NONE;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    vkCreateSampler(vkDevice, &createInfo, nullptr, &sampler);
}

void VkContext::createShaders(const VkDevice& vkDevice){
    std::vector<uint32_t> vertexCode, fragmentCode;
    uint32_t lengthInBytes;
    uint32_t* _vertexCode = (uint32_t*)platform_read_file2("vertex.spv", &lengthInBytes);
    vertexCode = std::vector<uint32_t>(_vertexCode, _vertexCode + lengthInBytes / sizeof(uint32_t));
    uint32_t* _fragmentCode = (uint32_t*)platform_read_file2("fragment.spv", &lengthInBytes);
    fragmentCode = std::vector<uint32_t>(_fragmentCode, _fragmentCode + lengthInBytes / sizeof(uint32_t));

    VkShaderCreateInfoEXT createInfos[2]{};

    createInfos[0].sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
    createInfos[0].flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
    createInfos[0].codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
    createInfos[0].pName = "main";
    createInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    createInfos[0].nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
    createInfos[0].codeSize = sizeof(uint32_t) * vertexCode.size();
    createInfos[0].pCode = vertexCode.data();
    createInfos[0].setLayoutCount = 1;
    createInfos[0].pSetLayouts = &descriptorSetLayout;
    createInfos[0].pushConstantRangeCount = 1;
    createInfos[0].pPushConstantRanges = &pushConstantRange;

    createInfos[1].sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
    createInfos[1].flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
    createInfos[1].codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
    createInfos[1].pName = "main";
    createInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    createInfos[1].codeSize = sizeof(uint32_t) * fragmentCode.size();
    createInfos[1].pCode = fragmentCode.data();
    createInfos[1].setLayoutCount = 1;
    createInfos[1].pSetLayouts = &descriptorSetLayout;
    createInfos[1].pushConstantRangeCount = 1;
    createInfos[1].pPushConstantRanges = &pushConstantRange;

    vkCreateShadersEXT(vkDevice, sizeof(createInfos) / sizeof(VkShaderCreateInfoEXT), createInfos, nullptr, shaders);
}

void VkContext::createTextureView(const VkDevice& vkDevice){
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


void VkContext::createUniformBuffers(const VkDevice& vkDevice) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    vkBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    vkDeviceMemory.resize(MAX_FRAMES_IN_FLIGHT);
    vkBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(vkDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vkBuffers[i], vkDeviceMemory[i]);
        vkMapMemory(vkDevice, vkDeviceMemory[i], 0, bufferSize, 0, &vkBuffersMapped[i]);
    }
}

void VkContext::createBuffer(const VkDevice& vkDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
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

uint32_t VkContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

void VkContext::updateUniformBuffer(const UniformBufferObject& ubo) {
    memcpy(vkBuffersMapped[0], &ubo, sizeof(ubo));
}

uint32_t VkContext::GetMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) const{
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