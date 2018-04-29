#pragma once

#include <Windows.h>
#include <string>
#include <functional>

class Application
{
	using KeyCallback = std::function<void(int key, int action, int mods)>;
	using ResizeCallback = std::function<void(int width, int height)>;
public:
	/*!
	 * @param instance A handle to the current instance of the application.
	 * @param cmd_show Controls how the window is to be shown. See the microsoft docs for more info.
	 * @param name Window title.
	 * @param width Initial window width.
	 * @param height Initial window height.
	 */
	Application(HINSTANCE instance, int cmd_show, std::string const& name, unsigned int width, unsigned int height);
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator=(Application&&) = delete;

	/*! Handles window events. Should be called every frame */
	void PollEvents();
	/*! Requests to close the window */
	void Stop();

	/*! Used to set the key callback function */
	void SetKeyCallback(KeyCallback callback);
	/*! Used to set the resize callback function */
	void SetResizeCallback(ResizeCallback callback);

	/*! Returns whether the application is running. (used for the main loop) */
	bool IsRunning() const;
	/*! Returns the native window handle (HWND)*/
	HWND GetWindowHandle() const;

private:
	/*! WindowProc that calls `WindowProc_Impl` */
	static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
	/*! Main WindowProc function */
	LRESULT CALLBACK WindowProc_Impl(HWND, UINT, WPARAM, LPARAM);

	KeyCallback m_key_callback;
	ResizeCallback m_resize_callback;

	bool m_running;
	HWND m_handle;
};