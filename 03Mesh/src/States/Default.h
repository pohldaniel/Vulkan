#pragma once

#include <vulkan/vulkan.h>
#include <engine/input/MouseEventListener.h>
#include <engine/input/KeyboardEventListener.h>
#include <engine/Camera.h>
#include <engine/ObjModel.h>

#include <States/StateMachine.h>

#include <Vulkan/Data.h>
#include <Vulkan/VlkTexture.h>
#include <Vulkan/VlkBuffer.h>
#include <Vulkan/VlkMesh.h>

class Default : public State, public MouseEventListener, public KeyboardEventListener {

public:

	Default(StateMachine& machine);
	~Default();

	void fixedUpdate() override;
	void update() override;
	void render() override;
	void resize(int deltaW, int deltaH) override;
	void OnMouseMotion(Event::MouseMoveEvent& event) override;
	void OnMouseWheel(Event::MouseWheelEvent& event) override;
	void OnMouseButtonDown(Event::MouseButtonEvent& event) override;
	void OnMouseButtonUp(Event::MouseButtonEvent& event) override;
	void OnKeyDown(Event::KeyboardEvent& event) override;
	void OnKeyUp(Event::KeyboardEvent& event) override;

private:

	void renderUi();

	bool m_initUi = true;
	bool m_drawUi = true;
	bool m_drawWirframe = false;

	Camera m_camera;
	UniformBufferObject m_uniformBufferObject;
	ObjModel m_model;

	VkBuffer m_srcVertexBuffer;
	VkDeviceMemory m_srcVertexBufferMemory;

	VkBuffer m_srcIndexBuffer;
	VkDeviceMemory m_srcIndexBufferMemory;

	VkBuffer m_dstVertexBuffer;
	VkDeviceMemory m_dstVertexBufferMemory;

	VkBuffer m_dstIndexBuffer;
	VkDeviceMemory m_dstIndexBufferMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureView;

	std::vector<VlkTexture> m_textures;
	std::list<VlkBuffer> m_vertexBuffer;
	std::list<VlkBuffer> m_indexBuffer;
	std::list<VlkMesh*> m_meshes;

	VlkMesh* m_mesh;

	VlkBuffer _vertex;
	VlkBuffer _index;

};