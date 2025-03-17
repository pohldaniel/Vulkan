#pragma once
#include <vulkan/vulkan.h>
#include <deque>
#include <functional>
#include <vector>

struct SurfaceDetails {
    VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};


class Swapchain {
public:

    void build(
        VkDevice logicalDevice,
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface,
        uint32_t width, uint32_t height,
        std::deque<std::function<void(VkDevice)>>& deviceDeletionQueue);

    uint32_t imageCount;
    VkSwapchainKHR chain;
    VkSurfaceFormatKHR format;
    VkExtent2D extent;

private:

    SurfaceDetails query_surface_support(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    VkExtent2D choose_extent(uint32_t width, uint32_t height, VkSurfaceCapabilitiesKHR capabilities);
    VkPresentModeKHR choose_present_mode(std::vector<VkPresentModeKHR> presentModes);
    VkSurfaceFormatKHR choose_surface_format(std::vector<VkSurfaceFormatKHR> formats);
};