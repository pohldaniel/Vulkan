#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

#include <vulkan/vulkan.h>
#include <Vulkan/VlkContext.h>
#include <Vulkan/VlkSwapchain.h>
#include <Vulkan/VlkSwapchainElement.h>
#include <engine/Material.h>

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

	m_model.loadModel("res/models/dragon/dragon.obj", glm::vec3(1.0f, 0.0f, 0.0f), -90.0f, glm::vec3(0.0f, 0.0f, 0.0f), 1.0f, false);

	for (ObjMesh* mesh : m_model.getMeshes()) {
		m_vertexBuffer.push_back(VlkBuffer());
		m_vertexBuffer.back().createBuffer(reinterpret_cast<const void*>(mesh->getVertexBuffer().data()), sizeof(float) * mesh->getVertexBuffer().size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
			
		m_indexBuffer.push_back(VlkBuffer());
		m_indexBuffer.back().createBuffer(reinterpret_cast<const void*>(mesh->getIndexBuffer().data()), sizeof(unsigned int) * mesh->getIndexBuffer().size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		
		m_textures.push_back(VlkTexture());
		m_textures.back().loadFromFile(mesh->getMaterial().textures[0], true);

		m_meshes.push_back(VlkMesh(m_vertexBuffer.back(), m_indexBuffer.back(), m_textures.back(), mesh->getIndexBuffer().size()));
		m_meshes.back().setShader(vlkContext.shader);
	}
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

	m_ubo.proj = m_camera.getPerspectiveMatrix();
	m_ubo.view = m_camera.getViewMatrix();
	m_ubo.model = glm::mat4(1.0f);

	memcpy(vlkContext.uniformMappingMVP, &m_ubo, sizeof(m_ubo));
}

void Default::render() {
	if (m_drawUi)
		renderUi();
	vlkDraw(m_meshes);
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
	m_camera.perspective(45.0f, static_cast<float>(Application::Width) / static_cast<float>(Application::Height), 0.1f, 1000.0f);
	m_camera.orthographic(0.0f, static_cast<float>(Application::Width), 0.0f, static_cast<float>(Application::Height), -1.0f, 1.0f);
}

void Default::renderUi() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("InvisibleWindow", nullptr, windowFlags);
	ImGui::PopStyleVar(3);

	ImGuiID dockSpaceId = ImGui::GetID("MainDockSpace");
	ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();

	if (m_initUi) {
		m_initUi = false;
		ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Left, 0.2f, nullptr, &dockSpaceId);
		ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Right, 0.2f, nullptr, &dockSpaceId);
		ImGuiID dock_id_down = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Down, 0.2f, nullptr, &dockSpaceId);
		ImGuiID dock_id_up = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Up, 0.2f, nullptr, &dockSpaceId);
		ImGui::DockBuilderDockWindow("Settings", dock_id_left);
	}

	// render widgets
	ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	if (ImGui::Checkbox("Draw Wirframe", &m_drawWirframe)) {
		vlkToggleWireframe();
	}
	ImGui::End();

	ImGui::Render();
	//ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData());
}