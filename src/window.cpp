#include "window.hpp"

#include <iostream>
#include <string>
#include <stdexcept>

#include "imgui\imgui.h"
#include "imgui\imgui_impl_dx12.h"

Application::Application(HINSTANCE instance, int cmd_show, std::string const& name, const unsigned int width, const unsigned int height)
{

	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = &Application::WindowProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = instance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = name.c_str();
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		throw std::runtime_error("Failed to register extended window class: ");
	}

	auto window_style = WS_OVERLAPPEDWINDOW;

	if (/*!allow_resizing*/ false)
	{
		window_style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	}

	RECT client_rect;
	client_rect.left = 0;
	client_rect.right = width;
	client_rect.top = 0;
	client_rect.bottom = height;
	AdjustWindowRectEx(&client_rect, window_style, FALSE, wc.style);

	m_handle = CreateWindowEx(
		NULL,
		name.c_str(),
		name.c_str(),
		window_style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		client_rect.right - client_rect.left, client_rect.bottom - client_rect.top,
		nullptr,
		nullptr,
		instance,
		nullptr
	);

	if (!m_handle)
	{
		throw std::runtime_error("Failed to create window." + GetLastError());
	}

	SetWindowLongPtr(m_handle, GWLP_USERDATA, (LONG_PTR)this);
	SetWindowLong(m_handle, GWL_EXSTYLE, GetWindowLong(m_handle, GWL_EXSTYLE) | WS_EX_LAYERED);

	ShowWindow(m_handle, cmd_show);
	UpdateWindow(m_handle);

	m_running = true;
}

Application::~Application()
{

}

void Application::PollEvents()
{
	MSG msg;
	if (PeekMessage(&msg, m_handle, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			m_running = false;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void Application::Stop()
{
	DestroyWindow(m_handle);
}

void Application::SetKeyCallback(KeyCallback callback)
{
	m_key_callback = callback;
}

void Application::SetResizeCallback(ResizeCallback callback)
{
	m_resize_callback = callback;
}

bool Application::IsRunning() const
{
	return m_running;
}

HWND Application::GetWindowHandle() const
{
	return m_handle;
}

LRESULT CALLBACK Application::WindowProc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
	Application* window = (Application*)GetWindowLongPtr(handle, GWLP_USERDATA);
	if (window) return window->WindowProc_Impl(handle, msg, w_param, l_param);
	return DefWindowProc(handle, msg, w_param, l_param);
}

constexpr unsigned char PercentageToAlpha(int percentage)
{
	return static_cast<unsigned char>(255.f * percentage / 100);
}

LRESULT CALLBACK Application::WindowProc_Impl(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
	if (ImGui_ImplWin32_WndProcHandler(handle, msg, w_param, l_param))
		return true;

	switch (msg)
	{
	case WM_DESTROY:
		m_running = false;
		PostQuitMessage(0);
		return 0;
	case WM_SETFOCUS:
	{
		constexpr auto alpha = PercentageToAlpha(100);
		SetLayeredWindowAttributes(handle, 0, alpha, LWA_ALPHA);
		return 0;
	}
	case WM_KILLFOCUS:
	{
		constexpr auto alpha = PercentageToAlpha(50);
		SetLayeredWindowAttributes(handle, 0, alpha, LWA_ALPHA);
		return 0;
	}
	case WM_KEYDOWN:
	case WM_KEYUP:
		if (m_key_callback)
		{
			m_key_callback((int)w_param, msg, 0);
		}

		return 0;
	case WM_SIZE:
		if (w_param != SIZE_MINIMIZED)
		{
			if (RECT rect; m_resize_callback && GetWindowRect(handle, &rect))
			{
				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				m_resize_callback(width, height);
			}
		}
		return 0;
	}

	return DefWindowProc(handle, msg, w_param, l_param);
}