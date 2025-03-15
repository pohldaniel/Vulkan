#include "instance.h"
#include <sstream>
#include <cstdlib>
#include <iostream>

bool supported_by_instance(const char** extensionNames, int extensionCount, const char** layerNames, int layerCount) {

	std::stringstream lineBuilder;

	//check extension support
	std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties().value;

	std::cout << "Instance can support the following extensions:" << std::endl;

	bool found;
	for (int i = 0; i < extensionCount; ++i) {
		const char* extension = extensionNames[i];
		found = false;
		for (vk::ExtensionProperties supportedExtension : supportedExtensions) {
			if (strcmp(extension, supportedExtension.extensionName) == 0) {
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
	std::vector<vk::LayerProperties> supportedLayers = vk::enumerateInstanceLayerProperties().value;

	std::cout << "Instance can support the following layers:" << std::endl;

	for (int i = 0; i < layerCount; ++i) {
		const char* layer = layerNames[i];
		found = false;
		for (vk::LayerProperties supportedLayer : supportedLayers) {
			if (strcmp(layer, supportedLayer.layerName) == 0) {
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

vk::Instance make_instance(const char* applicationName, std::deque<std::function<void(vk::Instance)>>& deletionQueue) {

	std::cout << "Making an instance..." << std::endl;

	uint32_t version = vk::enumerateInstanceVersion().value;



	// set the patch to 0 for best compatibility/stability)
	version &= ~(0xFFFU);

	vk::ApplicationInfo appInfo = vk::ApplicationInfo(
		applicationName,
		version,
		"Doing it the hard way",
		version,
		version
	);

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
	vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo(
		vk::InstanceCreateFlags(),
		&appInfo,
		1, instanceLayersList,
		3, instanceExtensionsList
	);

	vk::ResultValue<vk::Instance> instanceAttempt= vk::createInstance(createInfo);
	if (instanceAttempt.result != vk::Result::eSuccess) {
		std::cout << "Failed to create Instance!" << std::endl;
		return nullptr;
	}

	vk::Instance instance = instanceAttempt.value;

	deletionQueue.push_back([](vk::Instance instance) {
		instance.destroy();
		std::cout << "Deleted Instance!" << std::endl;
		});
	

	return instance;
}