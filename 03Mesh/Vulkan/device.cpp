#include "device.h"

#define ArraySize(arr) sizeof((arr)) / sizeof((arr[0]))

bool supports(
    const VkPhysicalDevice& device,
	const char** ppRequestedExtensions,
	const uint32_t requestedExtensionCount) {

	uint32_t amountOfDeviceExtensions = 0;
	vkEnumerateDeviceExtensionProperties(device, NULL, &amountOfDeviceExtensions, NULL);
	VkExtensionProperties* deviceExtensions = new VkExtensionProperties[amountOfDeviceExtensions];
	vkEnumerateDeviceExtensionProperties(device, NULL, &amountOfDeviceExtensions, deviceExtensions);

	/*std::cout << "Physical Device Supported Extensions:" << std::endl;
	for (uint32_t i = 0; i < requestedExtensionCount; ++i) {
        bool supported = false;

	for (size_t j = 0; j < amountOfDeviceExtensions; i++) {
        std::string name = deviceExtensions[j].extensionName;
		if (!name.compare(ppRequestedExtensions[i])) {
            supported = true;
            break;
        }
    }
        if (!supported) {
            return false;
        }
    }*/
    return true;
}

bool is_suitable(const VkPhysicalDevice& device) {
	const char* ppRequestedExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	if (supports(device, &ppRequestedExtension, 1)) {

	}else {
		std::cout << "Device can't support the requested extensions!" << std::endl;
		return false;
	}
	return true;
}

VkPhysicalDevice choose_physical_device(const VkInstance& instance) {

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, 0);
	VkPhysicalDevice* devices = new VkPhysicalDevice[deviceCount];
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
	for (size_t i = 0; i < deviceCount; i++) {

		if (is_suitable(devices[i])) {
			return devices[i];
		}
	}

	return nullptr;
}

uint32_t find_queue_family_index(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkQueueFlags queueType) {
	
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, NULL);
	VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[count];
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queueFamilies);

	for (size_t i = 0; i < count; ++i) {
		VkQueueFamilyProperties queueFamily = queueFamilies[i];

		VkBool32 canPresent = false;
		if (surface) {
			if ( vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &canPresent) != VkResult::VK_SUCCESS) {
				canPresent = true;
			}
		}
		else {
			canPresent = true;
		}

		bool supported = false;
		if (queueFamily.queueFlags & queueType) {
			supported = true;
		}

		if (supported && canPresent) {
			return i;
		}
	}
	return UINT32_MAX;
}

VkDevice create_logical_device(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::deque<std::function<void(VkDevice)>>& deletionQueue) {
	
	float queuePriority = 1.0f;
	uint32_t graphicsIndex = find_queue_family_index(physicalDevice, surface, VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT);
	VkDeviceQueueCreateInfo queueInfo = {};
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueFamilyIndex = graphicsIndex;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &queuePriority;

	const char* deviceExtensionsList[] = {
		"VK_KHR_dynamic_rendering",
		"VK_EXT_shader_object",
		"VK_KHR_swapchain"
	};

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	
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

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.ppEnabledExtensionNames = deviceExtensionsList;
	deviceInfo.enabledExtensionCount = ArraySize(deviceExtensionsList);
	//deviceInfo.pEnabledFeatures = &deviceFeatures;
	deviceInfo.pEnabledFeatures = NULL;
	deviceInfo.pNext = &deviceSynchronization2Features;

	VkDevice device = nullptr;
	auto result = vkCreateDevice(physicalDevice, &deviceInfo, 0, &device);
	
	if (result == VkResult::VK_SUCCESS) {
		deletionQueue.push_back([](VkDevice device) {
			vkDestroyDevice(device, NULL);
		});
	}else {
		std::cout << "Device creation failed!" << std::endl;
	}
	return device;
}