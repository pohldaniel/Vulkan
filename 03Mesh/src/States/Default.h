#pragma once

#include <vulkan/vulkan.h>
#include <engine/input/MouseEventListener.h>
#include <engine/input/KeyboardEventListener.h>
#include <engine/Camera.h>
#include <engine/ObjModel.h>

#include <States/StateMachine.h>

#include <Vulkan/Data.h>

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
};