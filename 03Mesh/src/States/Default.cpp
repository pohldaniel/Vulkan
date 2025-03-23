#include <vulkan/vulkan.h>
#include "Vulkan/VlkContext.h"

#include "Default.h"
#include "Application.h"
#include "Globals.h"


Default::Default(StateMachine& machine) : State(machine, States::DEFAULT) {

	Application::SetCursorIcon(IDC_ARROW);
	EventDispatcher::AddKeyboardListener(this);
	EventDispatcher::AddMouseListener(this);

	m_camera.perspective(45.0f, static_cast<float>(Application::Width) / static_cast<float>(Application::Height), 0.1f, 1000.0f);
	m_camera.orthographic(0.0f, static_cast<float>(Application::Width), 0.0f, static_cast<float>(Application::Height), -1.0f, 1.0f);
	m_camera.lookAt(glm::vec3(0.0f, 10.0f, 30.0f), glm::vec3(0.0f, 10.0f, 30.0f) + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_camera.setRotationSpeed(0.1f);
	m_camera.setMovingSpeed(10.0f);

	m_model.loadModel("res/models/dragon/dragon.obj", glm::vec3(1.0f, 0.0f, 0.0f), -90.0f, glm::vec3(0.0f, 0.0f, 0.0f), 1.0f, true);

	uint32_t size = sizeof(float) * m_model.getVertexBuffer().size();
	vlkCreateBuffer(m_srcVertexBuffer, m_srcVertexBufferMemory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vlkMapBuffer(m_srcVertexBufferMemory, reinterpret_cast<const void*>(m_model.getVertexBuffer().data()), size);
	vlkCreateBuffer(m_dstVertexBuffer, m_dstVertexBufferMemory, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vlkCopyBuffer(m_srcVertexBuffer, m_dstVertexBuffer, size);

	size = sizeof(unsigned int) * m_model.getIndexBuffer().size();
	vlkCreateBuffer(m_srcIndexBuffer, m_srcIndexBufferMemory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vlkMapBuffer(m_srcIndexBufferMemory, reinterpret_cast<const void*>(m_model.getIndexBuffer().data()), size);
	vlkCreateBuffer(m_dstIndexBuffer, m_dstIndexBufferMemory, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vlkCopyBuffer(m_srcIndexBuffer, m_dstIndexBuffer, size);
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

	Application::VlkContext.ubo.proj = m_camera.getPerspectiveMatrix();
	Application::VlkContext.ubo.view = m_camera.getViewMatrix();
	Application::VlkContext.ubo.model = glm::mat4(1.0f);
}

void Default::render() {
	vlkDraw(Application::VlkContext, m_dstVertexBuffer, m_dstIndexBuffer, static_cast<uint32_t>(m_model.getIndexBuffer().size()));
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