#include "instance.h"
#include <sstream>
#include <cstdlib>
#include <iostream>

bool supported_by_instance(const char** extensionNames, int extensionCount, const char** layerNames, int layerCount) {

	std::stringstream lineBuilder;

	uint32_t amountOfInstanceExtensions = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &amountOfInstanceExtensions, NULL);
	VkExtensionProperties* instanceExtensions = new VkExtensionProperties[amountOfInstanceExtensions];
	vkEnumerateInstanceExtensionProperties(NULL, &amountOfInstanceExtensions, instanceExtensions);

	bool found;
	for (int i = 0; i < extensionCount; ++i) {
		const char* extension = extensionNames[i];
		found = false;
		for (size_t i = 0; i < amountOfInstanceExtensions; i++) {
			if (strcmp(extension, instanceExtensions[i].extensionName) == 0) {
				found = true;
				std::cout << lineBuilder.str() << std::endl;
				lineBuilder.str("");
				break;
			}
		}
		if (!found) {
			lineBuilder << "Extension \"" << extension << "\" is not supported!";
			std::cout << lineBuilder.str() << std::endl;
			return false;
		}
	}

	uint32_t amountOfInstanceLayers = 0;
	vkEnumerateInstanceLayerProperties(&amountOfInstanceLayers, NULL);
	VkLayerProperties* instanceLayers = new VkLayerProperties[amountOfInstanceLayers];
	vkEnumerateInstanceLayerProperties(&amountOfInstanceLayers, instanceLayers);

	for (int i = 0; i < layerCount; ++i) {
		const char* layer = layerNames[i];
		found = false;
		for (size_t i = 0; i < amountOfInstanceLayers; i++) {
			if (strcmp(layer, instanceLayers[i].layerName) == 0) {
				found = true;
				std::cout << lineBuilder.str() << std::endl;;
				lineBuilder.str("");
				break;
			}
		}
		if (!found) {
			lineBuilder << "Layer \"" << layer << "\" is not supported!";
			std::cout << lineBuilder.str() << std::endl;
			return false;
		}
	}

	return true;
}

VkInstance make_instance(const char* applicationName, std::deque<std::function<void(VkInstance)>>& deletionQueue) {

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "applicationName";
	appInfo.pEngineName = "Doing it the hard way";
	appInfo.apiVersion = VK_API_VERSION_1_4;

	const char* instanceExtensionsList[] = {
		"VK_KHR_win32_surface",
		"VK_EXT_debug_utils",
		"VK_KHR_surface",
	};

	const char* instanceLayersList[]{
		"VK_LAYER_KHRONOS_validation",
	};

	if (!supported_by_instance(instanceExtensionsList, 3, instanceLayersList, 1)) {
		return nullptr;
	}

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.ppEnabledExtensionNames = instanceExtensionsList;
	instanceInfo.enabledExtensionCount = 3;
	instanceInfo.ppEnabledLayerNames = instanceLayersList;
	instanceInfo.enabledLayerCount = 1;

	VkInstance instance;
	vkCreateInstance(&instanceInfo, 0, &instance);

	deletionQueue.push_back([](VkInstance instance) {
		vkDestroyInstance(instance, NULL);
		std::cout << "Deleted Instance!" << std::endl;
	});
	

	return instance;
}