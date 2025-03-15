#pragma once
#define GLFW_INCLUDE_VULKAN

#include <deque>
#include <functional>
#include "instance.h"
#include "frame.h"
#include "swapchain.h"
#include <vector>

/**
 * @brief Vroom vroom.
 *
 */
class Engine {

public:

    /**
     * @brief Construct a new Engine object
     *
     * @param window main window to render to
     */
    Engine(void* window, int width, int height);

    /**
     * @brief Destroy the Engine object
     *
     */
    ~Engine();

    /**
    * @brief draw something!
    */
    void draw();

private:

    /**
     * @brief static debug logger
     *
     */
   

    /**
     * @brief Main window
     *
     */


    /**
    * @brief Stores destructors!
    */
    std::deque<std::function<void(vk::Instance)>> instanceDeletionQueue;
    std::deque<std::function<void(vk::Device)>> deviceDeletionQueue;

    /**
    * @brief the main instance
    */
    vk::Instance instance;

    /**
    * @brief dynamic instance dispatcher
    */
    vk::detail::DispatchLoaderDynamic dldi;

    /**
    * @brief Debug messenger
    */
    //vk::DebugUtilsMessengerEXT debugMessenger = nullptr;

    /**
     * @brief A physical device
     * 
     */
    vk::PhysicalDevice physicalDevice;

    /**
     * @brief An abstraction of the physical device
     * 
     */
    vk::Device logicalDevice;

    /**
     * @brief Queues for work submission
     * 
     */
    vk::Queue graphicsQueue;

    /**
     * @brief Surface to present to
     * 
     */
    vk::SurfaceKHR surface;

    /**
     * @brief The engine's swapchain
     * 
     */
    Swapchain swapchain;

    /**
     * @brief Frames used for rendering
     * 
     */
    std::vector<Frame> frames;

    /**
     * @brief shader objects
     * 
     */
    std::vector<vk::ShaderEXT> shaders;

    /**
    * @brief memory pool for command buffer allocation
    */
    vk::CommandPool commandPool;

    /**
    * @brief Rendering info
    */
    vk::RenderingInfoKHR renderingInfo;
};