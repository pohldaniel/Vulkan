#include <iostream>
#include <time.h>
#include <States/Default.h>

#include "Application.h"
#include "Globals.h"
#include "Vulkan/VlkExtension.h"

EventDispatcher& Application::EventDispatcher = EventDispatcher::Get();
StateMachine* Application::Machine = nullptr;
HWND Application::Window;
int Application::Width;
int Application::Height;
bool Application::InitWindow = false;
bool Application::Fullscreen = false;
bool Application::Init = false;
DWORD Application::SavedExStyle;
DWORD Application::SavedStyle;
RECT Application::Savedrc;
//VlkContext Application::VlkContext = {};

VkInstance instance = nullptr;

HCURSOR Application::Cursor = LoadCursor(nullptr, IDC_ARROW);
HICON Application::Icon = (HICON)LoadImage(NULL, "res/icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);

Application::Application(const float& dt, const float& fdt) : m_dt(dt), m_fdt(fdt) {
	Width = WIDTH;
	Height = HEIGHT;

	createWindow();	
	showWindow();
	initVulkan();
	
	EventDispatcher.setProcessOSEvents([&]() {
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) return false;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return true;
	});
	Init = true;
	SavedExStyle = GetWindowLong(Window, GWL_EXSTYLE);
	SavedStyle = GetWindowLong(Window, GWL_STYLE);
	initStates();	
}
Application::~Application() {	
	delete Machine;
	HDC hdc = GetDC(Window);
	ReleaseDC(Window, hdc);
	UnregisterClass("WINDOWCLASS", (HINSTANCE)GetModuleHandle(NULL));
}

void Application::createWindow() {
	
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = StaticWndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = (HINSTANCE)GetModuleHandle(NULL);
	windowClass.hIcon = (HICON)Icon;
	windowClass.hCursor = (HCURSOR)Cursor;
	windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = "WINDOWCLASS";
	windowClass.hIconSm = (HICON)Icon;

	if (!RegisterClassEx(&windowClass))
		return;

	Window = CreateWindowEx(
		NULL,
		"WINDOWCLASS",
		"Monster Hunter",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		Width,
		Height,
		NULL,
		NULL,
		windowClass.hInstance,
		this);

	if (!Window)
		return;

}

void Application::showWindow() {
	RECT rect1;
	GetWindowRect(Window, &rect1);
	RECT rect2;
	GetClientRect(Window, &rect2);

	SetWindowPos(Window, NULL, rect1.left, rect1.top, Width + ((rect1.right - rect1.left) - (rect2.right - rect2.left)), Height + ((rect1.bottom - rect1.top) - (rect2.bottom - rect2.top)), NULL);

	ShowWindow(Window, SW_SHOW);
	UpdateWindow(Window);

	InitWindow = true;
}

LRESULT CALLBACK Application::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	Application* application = nullptr;

	switch (message) {
		case WM_CREATE: {
			application = static_cast<Application*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
			break;
		}default: {
			application = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
			break;
		}
	}

	if (wParam == SC_KEYMENU && (lParam >> 16) <= 0) {
		return 0;
	}

	if (application) {
		application->processEvent(hWnd, message, wParam, lParam);
		return application->ApplicationWndProc(hWnd, message, wParam, lParam);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT Application::ApplicationWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {
		case WM_CREATE: {
			Keyboard::instance().enable();
			break;
		}case WM_DESTROY: {
			if (InitWindow) {
				PostQuitMessage(0);
			}
			break;
		}case WM_KEYDOWN: {
			switch (wParam) {
				case VK_RETURN: {
					if ((HIWORD(lParam) & KF_ALTDOWN))
						ToggleFullScreen(!Fullscreen);
					break;
				}
			}
			break;
		//ignore left alt
		}/*case WM_SYSKEYDOWN: {
			switch (wParam) {
				case VK_RETURN: {
					if ((HIWORD(lParam) & KF_ALTDOWN))
						ToggleFullScreen(!Fullscreen);
					break;
				}
			}
			break;
		}*/case WM_SIZE: {
			int deltaW = Width;
			int deltaH = Height;

			Height = HIWORD(lParam);		// retrieve width and height
			Width = LOWORD(lParam);

			deltaW = Width - deltaW;
			deltaH = Height - deltaH;

			if (Height == 0) {					// avoid divide by zero
				Height = 1;
			}
			Resize(deltaW, deltaH);

			break;
		}default: {
			//Mouse::instance().handleMsg(hWnd, message, wParam, lParam);
			break;
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void Application::initVulkan() {
	vlkInit(Window);
}

const HWND& Application::GetWindow() {
	return Window;
}

StateMachine* Application::GetMachine() {
	return Machine;
}

bool Application::isRunning() {
	return EventDispatcher.update();
}

void Application::render() {
	Machine->render();
}

void Application::update() {
	Mouse::instance().update();
	Keyboard::instance().update();
	Machine->update();

	if (!Machine->isRunning()) {
		SendMessage(Window, WM_DESTROY, NULL, NULL);
	}
}

void Application::fixedUpdate() {
	Machine->fixedUpdate();
}

void Application::initStates() {	
	Machine = new StateMachine(m_dt, m_fdt);
	//Machine->addStateAtTop(new Menu(*Machine));
	Machine->addStateAtTop(new Default(*Machine));
}

void Application::processEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {
	case WM_MOUSEMOVE: {
		Event event;
		event.type = Event::MOUSEMOTION;
		event.data.mouseMove.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
		event.data.mouseMove.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
		event.data.mouseMove.button = wParam & MK_RBUTTON ? Event::MouseMoveEvent::MouseButton::BUTTON_RIGHT : wParam & MK_LBUTTON ? Event::MouseMoveEvent::MouseButton::BUTTON_LEFT : Event::MouseMoveEvent::MouseButton::NONE;
		EventDispatcher.pushEvent(event);
		break;
	}case WM_LBUTTONDOWN: {
		Event event;
		event.type = Event::MOUSEBUTTONDOWN;
		event.data.mouseButton.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
		event.data.mouseButton.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
		event.data.mouseButton.button = Event::MouseButtonEvent::MouseButton::BUTTON_LEFT;
		EventDispatcher.pushEvent(event);
		break;
	}case WM_RBUTTONDOWN: {
		Event event;
		event.type = Event::MOUSEBUTTONDOWN;
		event.data.mouseButton.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
		event.data.mouseButton.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
		event.data.mouseButton.button = Event::MouseButtonEvent::MouseButton::BUTTON_RIGHT;
		EventDispatcher.pushEvent(event);
		break;
	}case WM_LBUTTONUP: {
		Event event;
		event.type = Event::MOUSEBUTTONUP;
		event.data.mouseButton.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
		event.data.mouseButton.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
		event.data.mouseButton.button = Event::MouseButtonEvent::MouseButton::BUTTON_LEFT;
		EventDispatcher.pushEvent(event);
		break;
	} case WM_RBUTTONUP: {
		Event event;
		event.type = Event::MOUSEBUTTONUP;
		event.data.mouseButton.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
		event.data.mouseButton.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
		event.data.mouseButton.button = Event::MouseButtonEvent::MouseButton::BUTTON_RIGHT;
		EventDispatcher.pushEvent(event);
		break;
	} case WM_KEYDOWN: {
		switch (wParam) {

		case VK_ESCAPE: {
			Event event;
			event.type = Event::KEYDOWN;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_SPACE: {
			Event event;
			event.type = Event::KEYDOWN;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_UP: {
			Event event;
			event.type = Event::KEYDOWN;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_DOWN: {
			Event event;
			event.type = Event::KEYDOWN;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_LEFT: {
			Event event;
			event.type = Event::KEYDOWN;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_RIGHT: {
			Event event;
			event.type = Event::KEYDOWN;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_CONTROL: {

			//Alt Graph
			//if (HIWORD(lParam) == 29)
			//	break;

			Event event;
			event.type = Event::KEYDOWN;
			event.data.keyboard.keyCode = (lParam & 0x01000000) != 0 ? VK_RCONTROL : VK_LCONTROL;
			EventDispatcher.pushEvent(event);
			break;
		}
		//case VK_MENU: {
		//	Event event;
		//	event.type = Event::KEYDOWN;
		//	event.data.keyboard.keyCode = VK_RMENU;
		//	EventDispatcher.pushEvent(event);
		//	break;
		//}
		//case VK_HOME: {
		//	Event event;
		//	event.type = Event::KEYDOWN;
		//	event.data.keyboard.keyCode = VK_HOME;
		//	EventDispatcher.pushEvent(event);
		//	break;
		//}
#if DEVBUILD
		case 'z': case 'Z': {
			vlkToggleWireframe();
			break;
		}case 'v': case 'V': {
			vlkToggleVerticalSync();
			break;
		}
#endif
		}
		break;
	}case WM_KEYUP: {

		switch (wParam) {

		case VK_ESCAPE: {
			Event event;
			event.type = Event::KEYUP;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_SPACE: {
			Event event;
			event.type = Event::KEYUP;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_UP: {
			Event event;
			event.type = Event::KEYUP;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_DOWN: {
			Event event;
			event.type = Event::KEYUP;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_LEFT: {
			Event event;
			event.type = Event::KEYUP;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_RIGHT: {
			Event event;
			event.type = Event::KEYUP;
			event.data.keyboard.keyCode = wParam;
			EventDispatcher.pushEvent(event);
			break;
		}case VK_CONTROL: {			
			Event event;
			event.type = Event::KEYUP;
			event.data.keyboard.keyCode = (lParam & 0x01000000) != 0 ? VK_RCONTROL : VK_LCONTROL;
			EventDispatcher.pushEvent(event);
			break;
		}
		}
		break;
	}case WM_SYSKEYDOWN: {

		switch (wParam) {
		case VK_MENU: {
			Event event;
			event.type = Event::KEYDOWN;
			event.data.keyboard.keyCode = VK_LMENU;
			EventDispatcher.pushEvent(event);
			break;
		}
		}
		break;
	}case WM_SYSKEYUP: {

		switch (wParam) {
		case VK_MENU: {
			Event event;
			event.type = Event::KEYUP;
			event.data.keyboard.keyCode = VK_LMENU;
			EventDispatcher.pushEvent(event);
			break;
		}
		}
		break;
	}case WM_MOUSEWHEEL: {
		Event event;
		event.type = Event::MOUSEWHEEL;
		event.data.mouseWheel.delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
		event.data.mouseWheel.direction = event.data.mouseWheel.delta > 0 ? Event::MouseWheelEvent::WheelDirection::UP : Event::MouseWheelEvent::WheelDirection::DOWN;
		EventDispatcher.pushEvent(event);
		break;
	}
	}
}

void Application::Resize(int deltaW, int deltaH) {
	if (Init) {
		vlkResize();
		Machine->getStates().top()->resize(deltaW, deltaH);
	}
}

void Application::ToggleFullScreen(bool isFullScreen, unsigned int width, unsigned int height) {

	int deltaW = width == 0u ? Width : width;
	int deltaH = height == 0u ? Height : height;

	if (isFullScreen) {

		if (!Fullscreen) {
			GetWindowRect(Window, &Savedrc);
		}
		Fullscreen = true;
		SetWindowLong(Window, GWL_EXSTYLE, 0);
		SetWindowLong(Window, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
		SetWindowPos(Window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

		Width = width == 0u ? GetSystemMetrics(SM_CXSCREEN) : width;
		Height = height == 0u ? GetSystemMetrics(SM_CYSCREEN) : height;

		deltaW = Width - deltaW;
		deltaH = Height - deltaH;

		SetWindowPos(Window, HWND_TOPMOST, 0, 0, Width, Height, SWP_SHOWWINDOW);
	}

	if (!isFullScreen) {

		Fullscreen = false;

		SetWindowLong(Window, GWL_EXSTYLE, SavedExStyle);
		SetWindowLong(Window, GWL_STYLE, SavedStyle);
		SetWindowPos(Window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		Width = (width == 0u || width == (Width + 16u)) ? Savedrc.right - Savedrc.left : width;
		Height = (height == 0u || height == (Height + 39u)) ? Savedrc.bottom - Savedrc.top : height;

		deltaW = Width - deltaW;
		deltaH = Height - deltaH;

		SetWindowPos(Window, HWND_NOTOPMOST, 0, 0, Width, Height, SWP_SHOWWINDOW);
	}
}

void Application::SetCursorIconFromFile(std::string file) {
	Application::Cursor = LoadCursorFromFileA(file.c_str());
	SetCursor(Cursor);
	SetClassLongPtr(Window, GCLP_HCURSOR, LONG_PTR(Cursor));
}

void Application::SetCursorIcon(LPCSTR resource) {
	Application::Cursor = LoadCursor(nullptr, resource);
	SetCursor(Cursor);
	SetClassLongPtr(Window, GCLP_HCURSOR, LONG_PTR(Cursor));
}

void  Application::SetCursorIcon(HCURSOR cursor) {
	Application::Cursor = cursor;
	SetCursor(Cursor);
	SetClassLongPtr(Window, GCLP_HCURSOR, LONG_PTR(Cursor));
}

void Application::loadAssets() {

}