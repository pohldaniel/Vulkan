#pragma once

#include <engine/input/MouseEventListener.h>
#include <engine/input/KeyboardEventListener.h>
#include <engine/CameraNew.h>

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

	CameraNew m_camera;
	UniformBufferObject m_uniformBufferObject;

};