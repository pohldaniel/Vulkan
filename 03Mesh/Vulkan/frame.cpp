#include "frame.h"
#include "image.h"

Frame::Frame(VkImage image, VkDevice logicalDevice, VkFormat swapchainFormat, std::deque<std::function<void(VkDevice)>>& deletionQueue) : 
	image(image), 
	commandBuffer(nullptr),
	imageView(nullptr){
    
    //imageView = create_image_view(logicalDevice, image, swapchainFormat);

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = swapchainFormat;
	createInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	vkCreateImageView(logicalDevice, &createInfo, NULL, &imageView);
    deletionQueue.push_back([this](VkDevice device) {
        vkDestroyImageView(device, imageView, nullptr);
    });
}

void Frame::set_command_buffer(VkInstance instance, VkCommandBuffer newCommandBuffer, std::vector<VkShaderEXT>& shaders, VkExtent2D frameSize) {
	auto vkCmdBindShadersEXT = (PFN_vkCmdBindShadersEXT)vkGetInstanceProcAddr(instance, "vkCmdBindShadersEXT");
	auto vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");
	auto vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");
	auto vkCmdSetVertexInputEXT = (PFN_vkCmdSetVertexInputEXT)vkGetInstanceProcAddr(instance, "vkCmdSetVertexInputEXT");

	commandBuffer = newCommandBuffer;

	build_color_attachment();
	build_rendering_info(frameSize);
	 
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	transition_image_layout(commandBuffer, image,
		VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED, VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
		VkAccessFlagBits::VK_ACCESS_NONE, VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	annoying_boilerplate_that_dynamic_rendering_was_meant_to_spare_us(instance, frameSize);

	vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);

	VkShaderStageFlagBits stages[2] = {
		VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,
		VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT
	};

	vkCmdSetVertexInputEXT(commandBuffer,0, NULL, 0, NULL);

	vkCmdBindShadersEXT(commandBuffer, 2, stages, shaders.data());
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderingKHR(commandBuffer);
	

	transition_image_layout(commandBuffer, image,
		VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VkAccessFlagBits::VK_ACCESS_NONE,
		VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

	vkEndCommandBuffer(commandBuffer);
	
}

void Frame::build_rendering_info(VkExtent2D frameSize) {

	/*
	* // Provided by VK_VERSION_1_3
		typedef struct VkRenderingInfo {
			VkStructureType                     sType;
			const void*                         pNext;
			VkRenderingFlags                    flags;
			VkRect2D                            renderArea;
			uint32_t                            layerCount;
			uint32_t                            viewMask;
			uint32_t                            colorAttachmentCount;
			const VkRenderingAttachmentInfo*    pColorAttachments;
			const VkRenderingAttachmentInfo*    pDepthAttachment;
			const VkRenderingAttachmentInfo*    pStencilAttachment;
		} VkRenderingInfo;
	*/

	VkRect2D rect;
	rect.offset = { 0,0 };
	rect.extent = frameSize;

	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.flags = VkRenderingFlagsKHR();
	renderingInfo.renderArea = rect;
	renderingInfo.layerCount = 1;
	//bitmask indicating the layers which will be rendered to
	renderingInfo.viewMask = 0;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
}

void Frame::build_color_attachment() {
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = imageView;
	colorAttachment.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue = VkClearValue({ 0.5f, 0.0f, 0.25f, 1.0f });
}

void Frame::annoying_boilerplate_that_dynamic_rendering_was_meant_to_spare_us(VkInstance instance, VkExtent2D frameSize) {
	auto vkCmdSetColorBlendEnableEXT = (PFN_vkCmdSetColorBlendEnableEXT)vkGetInstanceProcAddr(instance, "vkCmdSetColorBlendEnableEXT");
	auto vkCmdSetSampleMaskEXT = (PFN_vkCmdSetSampleMaskEXT)vkGetInstanceProcAddr(instance, "vkCmdSetSampleMaskEXT");
	auto vkCmdSetPolygonModeEXT = (PFN_vkCmdSetPolygonModeEXT)vkGetInstanceProcAddr(instance, "vkCmdSetPolygonModeEXT");
	auto vkCmdSetColorBlendEquationEXT = (PFN_vkCmdSetColorBlendEquationEXT)vkGetInstanceProcAddr(instance, "vkCmdSetColorBlendEquationEXT");
	auto vkCmdSetRasterizationSamplesEXT = (PFN_vkCmdSetRasterizationSamplesEXT)vkGetInstanceProcAddr(instance, "vkCmdSetRasterizationSamplesEXT");
	auto vkCmdSetAlphaToCoverageEnableEXT = (PFN_vkCmdSetAlphaToCoverageEnableEXT)vkGetInstanceProcAddr(instance, "vkCmdSetAlphaToCoverageEnableEXT");
	auto vkCmdSetColorWriteMaskEXT = (PFN_vkCmdSetColorWriteMaskEXT)vkGetInstanceProcAddr(instance, "vkCmdSetColorWriteMaskEXT");

	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = frameSize.width;
	viewport.height = frameSize.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewportWithCount(commandBuffer,1, &viewport);
	VkRect2D scissor;
	scissor.offset = { 0,0 };
	scissor.extent = frameSize;


	vkCmdSetScissorWithCount(commandBuffer, 1, &scissor);
	vkCmdSetRasterizerDiscardEnable(commandBuffer, 0);
	vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
	vkCmdSetRasterizationSamplesEXT(commandBuffer, VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT);


	uint32_t sampleMask = 1;
	vkCmdSetSampleMaskEXT(commandBuffer, VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT, &sampleMask);
	vkCmdSetAlphaToCoverageEnableEXT(commandBuffer, 0);
	vkCmdSetCullMode(commandBuffer, VkCullModeFlagBits::VK_CULL_MODE_NONE);
	vkCmdSetDepthTestEnable(commandBuffer, 0);
	vkCmdSetDepthWriteEnable(commandBuffer, 0);
	vkCmdSetDepthBiasEnable(commandBuffer, 0);
	vkCmdSetStencilTestEnable(commandBuffer, 0);
	vkCmdSetPrimitiveTopology(commandBuffer, VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	vkCmdSetPrimitiveRestartEnable(commandBuffer, 0);

	uint32_t colorBlendEnable = 0;
	vkCmdSetColorBlendEnableEXT(commandBuffer, 0, 1, &colorBlendEnable);


	VkColorBlendEquationEXT equation;
	equation.colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
	equation.dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
	equation.srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
	equation.alphaBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
	equation.dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
	equation.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
	vkCmdSetColorBlendEquationEXT(commandBuffer, 0, 1, &equation);

	VkColorComponentFlags colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
		| VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	vkCmdSetColorWriteMaskEXT(commandBuffer, 0, 1, &colorWriteMask);
}