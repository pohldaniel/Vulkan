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

	uint32_t size = sizeof(float) * m_model.getMesh(0)->getVertexBuffer().size();
	vlkCreateBuffer(m_srcVertexBuffer, m_srcVertexBufferMemory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vlkMapBuffer(m_srcVertexBufferMemory, reinterpret_cast<const void*>(m_model.getMesh(0)->getVertexBuffer().data()), size);
	vlkCreateBuffer(m_dstVertexBuffer, m_dstVertexBufferMemory, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vlkCopyBuffer(m_srcVertexBuffer, m_dstVertexBuffer, size);

	size = sizeof(unsigned int) * m_model.getMesh(0)->getIndexBuffer().size();
	vlkCreateBuffer(m_srcIndexBuffer, m_srcIndexBufferMemory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vlkMapBuffer(m_srcIndexBufferMemory, reinterpret_cast<const void*>(m_model.getMesh(0)->getIndexBuffer().data()), size);
	vlkCreateBuffer(m_dstIndexBuffer, m_dstIndexBufferMemory, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vlkCopyBuffer(m_srcIndexBuffer, m_dstIndexBuffer, size);

	const std::vector<Material>& materials = Material::GetMaterials();

	for (const Material& material : materials) {
		m_textures.push_back(VlkTexture());
		m_textures.back().loadFromFile(material.textures[0], true);
		m_textures.back().setDescriptorSet(vlkContext.descriptorSet);
		
	}
	m_textures[0].bind(1u);
	
	for (ObjMesh* mesh : m_model.getMeshes()) {
		m_vertexBuffer.push_back(VlkBuffer());
		m_vertexBuffer.back().createBufferVertex(reinterpret_cast<const void*>(mesh->getVertexBuffer().data()), sizeof(float) * mesh->getVertexBuffer().size());
			
		m_indexBuffer.push_back(VlkBuffer());
		m_indexBuffer.back().createBufferIndex(reinterpret_cast<const void*>(mesh->getIndexBuffer().data()), sizeof(unsigned int) * mesh->getIndexBuffer().size());
		
		m_meshes.push_back(new VlkMesh(m_vertexBuffer.back(), m_indexBuffer.back(), mesh->getIndexBuffer().size()));
		m_meshes.back()->createMVP(vlkContext.memoryAllocator, vlkContext.descriptorSet);
		m_meshes.back()->setShader(vlkContext.shader);
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

	vlkContext.ubo.proj = m_camera.getPerspectiveMatrix();
	vlkContext.ubo.view = m_camera.getViewMatrix();
	vlkContext.ubo.model = glm::mat4(1.0f);
}

void Default::render() {
	if (m_drawUi)
		renderUi();
	//vlkDraw(m_meshes);
	for (std::list<VlkMesh*>::const_iterator it = m_meshes.begin(); it != m_meshes.end(); ++it) {
		vlkDrawMesh(*it);
	}
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