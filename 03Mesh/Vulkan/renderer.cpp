#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#include <fstream>
#include <windows.h>
#include "renderer.h"
#include "device.h"
#include "swap_chain.h"
#include "shader.h"
#include "command.h"

static const int maxDescriptorSets = 10;
static const int maxDescriptorCount = 65536;
static const int pushConstantRangeSize = 128;

static std::string getFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file.good())
    {
        std::cerr << "Failed to read file: " << path << std::endl;
        return "";
    }

    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

static char* platform_read_file(const char* path, uint32_t* length) {
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

    _shaders = make_shader_objects(instance, logicalDevice,"_shader", deviceDeletionQueue);
  
	commandPool = make_command_pool(logicalDevice, graphicsQueueFamilyIndex,deviceDeletionQueue);

	for (uint32_t i = 0; i < scImgCount; ++i) {
		frames[i].set_command_buffer(instance, allocate_command_buffer(logicalDevice, commandPool), _shaders, swapchain.extent);
	}

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
	vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore);

    createAllocator();
    createMesh();
    createTexture();
    createDescriptorPool();
    createDescriptorSetLayout();
    createPushConstantRange();
    createPipelineLayout();
    createSampler();
    createShaders();
    createTextureView();
    //meshShaders = make_shader_objects(instance, logicalDevice, "mesh", deviceDeletionQueue);

    swapchain2 = new Swapchain2(this, width, height);
}

void Engine::draw() {

    bool shouldResize = swapchain2->draw();

    if (shouldResize)
    {
        resize();
    }

	/*vkAcquireNextImageKHR(logicalDevice, swapchain.chain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

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
	vkQueueWaitIdle(graphicsQueue);*/
}

void Engine::resize()
{
    /*VkResult result;

    vkDeviceWaitIdle(logicalDevice);

    delete swapchain2;
    swapchain2 = new Swapchain2(this, width, height);*/
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

void Engine::createDescriptorPool() {
    VkResult result;

    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxDescriptorSets * maxDescriptorCount },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxDescriptorSets * maxDescriptorCount }
    };

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.maxSets = maxDescriptorSets;
    createInfo.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
    createInfo.pPoolSizes = poolSizes;

    vkCreateDescriptorPool(logicalDevice, &createInfo, nullptr, &descriptorPool);
}

void Engine::createDescriptorSetLayout(){
    VkResult result;

    VkDescriptorSetLayoutBinding bindings[2]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = maxDescriptorCount;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = maxDescriptorCount;
    bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorBindingFlags bindingFlags[] = {
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

    vkCreateDescriptorSetLayout(logicalDevice, &createInfo, nullptr, &descriptorSetLayout);
}

void Engine::createPushConstantRange(){
    pushConstantRange = VkPushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.size = pushConstantRangeSize;
}

void Engine::createPipelineLayout() {
    VkResult result;

    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &descriptorSetLayout;
    createInfo.pushConstantRangeCount = 1;
    createInfo.pPushConstantRanges = &pushConstantRange;

    vkCreatePipelineLayout(logicalDevice, &createInfo, nullptr, &pipelineLayout);
}

void Engine::createSampler(){
    VkResult result;

    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.maxAnisotropy = 16;
    createInfo.maxLod = VK_LOD_CLAMP_NONE;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    vkCreateSampler(logicalDevice, &createInfo, nullptr, &sampler);
}

void Engine::createShaders()
{
    std::vector<uint32_t> vertexCode, fragmentCode;
    uint32_t lengthInBytes;
    uint32_t* _vertexCode = (uint32_t*)platform_read_file("vertex.spv", &lengthInBytes);
    vertexCode = std::vector<uint32_t>(_vertexCode, _vertexCode + lengthInBytes / sizeof(uint32_t));
    uint32_t* _fragmentCode = (uint32_t*)platform_read_file("fragment.spv", &lengthInBytes);
    fragmentCode = std::vector<uint32_t>(_fragmentCode, _fragmentCode + lengthInBytes / sizeof(uint32_t));


    auto vkCreateShadersEXT = (PFN_vkCreateShadersEXT)vkGetInstanceProcAddr(instance, "vkCreateShadersEXT");
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

    vkCreateShadersEXT(logicalDevice,
        sizeof(createInfos) / sizeof(VkShaderCreateInfoEXT),
        createInfos,
        nullptr,
        shaders
        );
}

void Engine::createTextureView()
{
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
    createInfo.image = texture.image;
    createInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCreateImageView(logicalDevice, &createInfo, nullptr, &textureView);
}
