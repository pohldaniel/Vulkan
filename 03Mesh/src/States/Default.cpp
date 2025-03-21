#include <vulkan/vulkan.h>
#include "Vulkan/VkContext.h"

#include "Default.h"
#include "Application.h"
#include "Globals.h"


Default::Default(StateMachine& machine) : State(machine, States::DEFAULT) {

	Application::SetCursorIcon(IDC_ARROW);
	EventDispatcher::AddKeyboardListener(this);
	EventDispatcher::AddMouseListener(this);

	m_camera.perspective(45.0f, static_cast<float>(Application::Width) / static_cast<float>(Application::Height), 0.1f, 1000.0f);
	m_camera.orthographic(0.0f, static_cast<float>(Application::Width), 0.0f, static_cast<float>(Application::Height), -1.0f, 1.0f);
	m_camera.lookAt(glm::vec3(0.0f, 2.0f, 10.0f), glm::vec3(0.0f, 2.0f, 10.0f) + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_camera.setRotationSpeed(0.1f);
	m_camera.setMovingSpeed(10.0f);

	m_model.loadModel("res/models/dragon/dragon.obj");
	m_model.loadModelGpu(Application::VkContext, m_srcVertexBuffer, m_srcVertexBufferMemory);

	uint32_t size = sizeof(float) * m_model.getMesh(0)->getVertexBuffer().size();
	void *pMem = nullptr;
	VkResult res = vkMapMemory(Application::VkContext.vkDevice, m_srcVertexBufferMemory, 0, size, 0, &pMem);
	if (res == VK_SUCCESS)
		std::cout << "SUCCESS: " << std::endl;
	memcpy(pMem, reinterpret_cast<const void*>(m_model.getMesh(0)->getVertexBuffer().data()), size);
	vkUnmapMemory(Application::VkContext.vkDevice, m_srcVertexBufferMemory);

	VkBufferCreateInfo vkBufferCreateInfo = {};
	vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferCreateInfo.size = size;
	vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(Application::VkContext.vkDevice, &vkBufferCreateInfo, NULL, &m_dstVertexBuffer);

	VkMemoryRequirements vkMemoryRequirements;
	vkGetBufferMemoryRequirements(Application::VkContext.vkDevice, m_dstVertexBuffer, &vkMemoryRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = vkMemoryRequirements.size;
	allocInfo.memoryTypeIndex = Application::VkContext.GetMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkAllocateMemory(Application::VkContext.vkDevice, &allocInfo, NULL, &m_dstVertexBufferMemory);
	vkBindBufferMemory(Application::VkContext.vkDevice, m_dstVertexBuffer, m_dstVertexBufferMemory, 0);



	VkCommandBufferAllocateInfo _allocInfo = {};
	_allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	_allocInfo.commandPool = Application::VkContext.vkCommandPool;
	_allocInfo.commandBufferCount = 1;
	_allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	vkAllocateCommandBuffers(Application::VkContext.vkDevice, &_allocInfo, &commandBuffer);


	VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
	vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkCommandBufferBeginInfo.pNext = NULL;	
	vkCommandBufferBeginInfo.pInheritanceInfo = NULL;

	vkBeginCommandBuffer(commandBuffer, &vkCommandBufferBeginInfo);

	VkBufferCopy vkBufferCopy = {};
	vkBufferCopy.srcOffset = 0;
	vkBufferCopy.dstOffset = 0;
	vkBufferCopy.size = size;

	vkCmdCopyBuffer(commandBuffer, m_srcVertexBuffer, m_dstVertexBuffer, 1, &vkBufferCopy);
	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = VK_NULL_HANDLE;
	submitInfo.pWaitDstStageMask = VK_NULL_HANDLE;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = VK_NULL_HANDLE;
	submitInfo.pNext = NULL;
	
	vkQueueSubmit(Application::VkContext.vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(Application::VkContext.vkQueue);
}

Default::~Default() {
	EventDispatcher::RemoveKeyboardListener(this);
	EventDispatcher::RemoveMouseListener(this);
}

void Default::fixedUpdate() {

}

void Default::update() {
	Keyboard& keyboard = Keyboard::instance();
	glm::vec3 direction = glm::vec3();

	float dx = 0.0f;
	float dy = 0.0f;
	bool move = false;

	if (keyboard.keyDown(Keyboard::KEY_W)) {
		direction += glm::vec3(0.0f, 0.0f, 1.0f);
		move |= true;
	}

	if (keyboard.keyDown(Keyboard::KEY_S)) {
		direction += glm::vec3(0.0f, 0.0f, -1.0f);
		move |= true;
	}

	if (keyboard.keyDown(Keyboard::KEY_A)) {
		direction += glm::vec3(-1.0f, 0.0f, 0.0f);
		move |= true;
	}

	if (keyboard.keyDown(Keyboard::KEY_D)) {
		direction += glm::vec3(1.0f, 0.0f, 0.0f);
		move |= true;
	}

	if (keyboard.keyDown(Keyboard::KEY_Q)) {
		direction += glm::vec3(0.0f, -1.0f, 0.0f);
		move |= true;
	}

	if (keyboard.keyDown(Keyboard::KEY_E)) {
		direction += glm::vec3(0.0f, 1.0f, 0.0f);
		move |= true;
	}

	Mouse& mouse = Mouse::instance();
	if (mouse.buttonDown(Mouse::MouseButton::BUTTON_RIGHT)) {
		dx = mouse.xDelta();
		dy = mouse.yDelta();
	}

	if (move || dx != 0.0f || dy != 0.0f) {
		if (dx || dy) {
			m_camera.rotate(dx, dy);

		}

		if (move) {
			m_camera.move(direction * m_dt);
		}
	}

	Application::VkContext.ubo.proj = m_camera.getPerspectiveMatrix();
	Application::VkContext.ubo.view = m_camera.getViewMatrix();
	Application::VkContext.ubo.model = glm::mat4(1.0f);

	//Application::vkContext.updateUniformBuffer(m_uniformBufferObject);
}

void Default::render() {
	vkDraw(Application::VkContext);
}

void Default::OnMouseMotion(Event::MouseMoveEvent& event) {

}

void Default::OnMouseButtonDown(Event::MouseButtonEvent& event) {
	if (event.button == 2u) {
		Mouse::instance().attach(Application::GetWindow());
	}
}

void Default::OnMouseButtonUp(Event::MouseButtonEvent& event) {
	if (event.button == 2u || event.button == 1u) {
		Mouse::instance().detach();
	}
}

void Default::OnMouseWheel(Event::MouseWheelEvent& event) {

}

void Default::OnKeyDown(Event::KeyboardEvent& event) {
	if (event.keyCode == VK_ESCAPE) {
		m_isRunning = false;
	}
}

void Default::OnKeyUp(Event::KeyboardEvent& event) {

}

void Default::resize(int deltaW, int deltaH) {
	
}