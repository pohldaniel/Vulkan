#include "device.h"

bool supports(
    const vk::PhysicalDevice& device,
	const char** ppRequestedExtensions,
	const uint32_t requestedExtensionCount) {
    
	std::cout << "Requested Physical Device Extensions:" << std::endl;
	std::cout << ppRequestedExtensions << "  "  << requestedExtensionCount << std::endl;

    std::vector<vk::ExtensionProperties> extensions = device.enumerateDeviceExtensionProperties().value;
	std::cout << "Physical Device Supported Extensions:" << std::endl;
	//std::cout << extensions << std::endl;

    for (uint32_t i = 0; i < requestedExtensionCount; ++i) {
        bool supported = false;

	for (vk::ExtensionProperties& extension : extensions) {
        std::string name = extension.extensionName;
		if (!name.compare(ppRequestedExtensions[i])) {
            supported = true;
            break;
        }
    }
        if (!supported) {
            return false;
        }
    }

    return true;
}

bool is_suitable(const vk::PhysicalDevice& device) {

	std::cout << "Checking if device is suitable" << std::endl;

	/*
	* A device is suitable if it can present to the screen, ie support
	* the swapchain extension
	*/
	const char* ppRequestedExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	if (supports(device, &ppRequestedExtension, 1)) {
		std::cout << "Device can support the requested extensions!" << std::endl;
	}
	else {
		std::cout << "Device can't support the requested extensions!" << std::endl;
		return false;
	}
	return true;
}

vk::PhysicalDevice choose_physical_device(const vk::Instance& instance) {

	std::cout << "Choosing physical device..." << std::endl;

	/*
	* ResultValueType<std::vector<PhysicalDevice, PhysicalDeviceAllocator>>::type
		Instance::enumeratePhysicalDevices( Dispatch const & d )

		std::vector<vk::PhysicalDevice> instance.enumeratePhysicalDevices( Dispatch const & d = static/default )
	*/
	std::vector<vk::PhysicalDevice> availableDevices = instance.enumeratePhysicalDevices().value;

	for (vk::PhysicalDevice device : availableDevices) {

		if (is_suitable(device)) {
			return device;
		}
	}

	return nullptr;
}

uint32_t find_queue_family_index(vk::PhysicalDevice physicalDevice,
	vk::SurfaceKHR surface,
    vk::QueueFlags queueType) {
	

	std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

	for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
		vk::QueueFamilyProperties queueFamily = queueFamilies[i];

		bool canPresent = false;
		if (surface) {
			if (physicalDevice.getSurfaceSupportKHR(i, surface)
					.result == vk::Result::eSuccess) {
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

vk::Device create_logical_device(
    vk::PhysicalDevice physicalDevice,
	vk::SurfaceKHR surface,
    std::deque<std::function<void(vk::Device)>>& deletionQueue) {
	
	uint32_t graphicsIndex = find_queue_family_index(physicalDevice, surface, vk::QueueFlagBits::eGraphics);
	float queuePriority = 1.0f;
	vk::DeviceQueueCreateInfo queueInfo = vk::DeviceQueueCreateInfo(
		vk::DeviceQueueCreateFlags(), graphicsIndex, 1, &queuePriority
	);

	vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures();
	vk::PhysicalDeviceShaderObjectFeaturesEXT shaderFeatures = vk::PhysicalDeviceShaderObjectFeaturesEXT(1);
	vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicFeatures = vk::PhysicalDeviceDynamicRenderingFeaturesKHR(1);
	shaderFeatures.pNext = &dynamicFeatures;

	uint32_t enabled_layer_count = 1;
	const char** ppEnabledLayers = nullptr;
	if (true) {
		enabled_layer_count = 1;
		ppEnabledLayers = (const char**) malloc(sizeof(const char*));
		ppEnabledLayers[0] = "VK_LAYER_KHRONOS_validation";
	}

	uint32_t enabledExtensionCount = 3;
	const char** ppEnabledExtensions = (const char**) malloc(enabledExtensionCount * sizeof(char*));
	ppEnabledExtensions[0] = "VK_KHR_swapchain";
	ppEnabledExtensions[1] = "VK_EXT_shader_object";
	ppEnabledExtensions[2] = "VK_KHR_dynamic_rendering";

	vk::DeviceCreateInfo deviceInfo = vk::DeviceCreateInfo(
		vk::DeviceCreateFlags(),
		1, &queueInfo,
		enabled_layer_count, ppEnabledLayers,
		enabledExtensionCount, ppEnabledExtensions,
		&deviceFeatures);
	deviceInfo.pNext = &shaderFeatures;
	
	vk::ResultValueType<vk::Device>::type logicalDevice = physicalDevice.createDevice(deviceInfo);
	vk::Device device = nullptr;
	if (logicalDevice.result == vk::Result::eSuccess) {
		std::cout << "GPU has been successfully abstracted!" << std::endl;

		deletionQueue.push_back([](vk::Device device) {
			device.destroy();
			std::cout << "Deleted logical device" << std::endl;
		});

		device = logicalDevice.value;
	}
	else {
		std::cout << "Device creation failed!" << std::endl;
	}

	if (ppEnabledLayers) {
		free(ppEnabledLayers);
	}
	free(ppEnabledExtensions);
	return device;
}