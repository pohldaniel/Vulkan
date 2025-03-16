#include "instance.h"
#include <sstream>
#include <cstdlib>
#include <iostream>

bool supported_by_instance(const char** extensionNames, int extensionCount, const char** layerNames, int layerCount) {

	std::stringstream lineBuilder;

	//check extension support
	uint32_t amountOfInstanceExtensions = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &amountOfInstanceExtensions, NULL);
	VkExtensionProperties* instanceExtensions = new VkExtensionProperties[amountOfInstanceExtensions];
	vkEnumerateInstanceExtensionProperties(NULL, &amountOfInstanceExtensions, instanceExtensions);

	std::cout << "Instance can support the following extensions:" << std::endl;

	bool found;
	for (int i = 0; i < extensionCount; ++i) {
		const char* extension = extensionNames[i];
		found = false;
		for (size_t i = 0; i < amountOfInstanceExtensions; i++) {
			if (strcmp(extension, instanceExtensions[i].extensionName) == 0) {
				found = true;
				lineBuilder << "Extension \"" << extension << "\" is supported!";
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

	//check layer support
	uint32_t amountOfInstanceLayers = 0;
	vkEnumerateInstanceLayerProperties(&amountOfInstanceLayers, NULL);
	VkLayerProperties* instanceLayers = new VkLayerProperties[amountOfInstanceLayers];
	vkEnumerateInstanceLayerProperties(&amountOfInstanceLayers, instanceLayers);

	std::cout << "Instance can support the following layers:" << std::endl;

	for (int i = 0; i < layerCount; ++i) {
		const char* layer = layerNames[i];
		found = false;
		for (size_t i = 0; i < amountOfInstanceLayers; i++) {
			if (strcmp(layer, instanceLayers[i].layerName) == 0) {
				found = true;
				lineBuilder << "Layer \"" << layer << "\" is supported!";
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

	std::cout << "Making an instance..." << std::endl;

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "applicationName";
	appInfo.pEngineName = "Doing it the hard way";
	appInfo.apiVersion = VK_API_VERSION_1_4;

	
	/*
	* Extensions
	*/
	//uint32_t glfwExtensionCount = 0;
	//const char** glfwExtensions;
	//glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	//uint32_t enabledExtensionCount = glfwExtensionCount;
	//if (true) {
	//	enabledExtensionCount++;
	//}
	//const char** ppEnabledExtensionNames = (const char**)malloc(enabledExtensionCount * sizeof(char*));

	uint32_t offset = 0;
	//for (;offset < glfwExtensionCount; ++offset) {
		//ppEnabledExtensionNames[offset] = glfwExtensions[offset];
	//}
	const char* instanceExtensionsList[] = {
		"VK_KHR_win32_surface",
		"VK_EXT_debug_utils",
		"VK_KHR_surface",
	};

	const char* instanceLayersList[]{
		"VK_LAYER_KHRONOS_validation",
	};

	std::cout << "extensions to be requested:" << std::endl;
	

	/*
	* Layers
	
	uint32_t enabledLayerCount = 0;
	if (true) {
		enabledLayerCount++;
	}
	const char** ppEnabledLayerNames = nullptr;
	if (enabledLayerCount > 0) {
		ppEnabledLayerNames = (const char**)malloc(enabledLayerCount * sizeof(char*));
	}

	if (true) {
		ppEnabledLayerNames[0] = "VK_LAYER_KHRONOS_validation";
	}*/

	std::cout << "layers to be requested:" << std::endl;
	

	if (!supported_by_instance(instanceExtensionsList, 3, instanceLayersList, 1)) {
		return nullptr;
	}

	/*
	*
	* from vulkan_structs.hpp:
	*
	* InstanceCreateInfo( VULKAN_HPP_NAMESPACE::InstanceCreateFlags     flags_                 = {},
										 const VULKAN_HPP_NAMESPACE::ApplicationInfo * pApplicationInfo_      = {},
										 uint32_t                                      enabledLayerCount_     = {},
										 const char * const *                          ppEnabledLayerNames_   = {},
										 uint32_t                                      enabledExtensionCount_ = {},
										 const char * const * ppEnabledExtensionNames_ = {} )
	*/
	

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