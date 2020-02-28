#include "Window.h"
#include <cassert>
#include <iostream>
#include <algorithm>
#include "AppConfig.h"

void RegisterWindowClass(HINSTANCE hInst , const wchar_t* windowClassName, WNDPROC Proc)
{
	WNDCLASSEXW windowClass   = {};
	windowClass.cbSize        = sizeof(WNDCLASSEXW);
	windowClass.style         = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc   = Proc;
	windowClass.cbClsExtra    = 0;
	windowClass.cbWndExtra    = 0;
	windowClass.hInstance     = hInst;
	windowClass.hIcon         = ::LoadIcon(hInst , NULL);
	windowClass.hIconSm       = ::LoadIcon(hInst , NULL);
	windowClass.hCursor       = ::LoadCursor(NULL , IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName  = NULL;
	windowClass.lpszClassName = windowClassName;

	static ATOM atom = ::RegisterClassExW(&windowClass);
	assert(atom > 0);
}

HWND CreateWindows(const wchar_t* windowClassName , HINSTANCE hInst , const wchar_t* windowTitle , uint32_t width , uint32_t height)
{
	int screenWidth  = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
	::AdjustWindowRect(&windowRect , WS_OVERLAPPEDWINDOW , FALSE);

	int windowWidth  = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	int windowX = std::max<int>(0 , (screenWidth  - windowWidth) / 2);
	int windowY = std::max<int>(0 , (screenHeight - windowHeight) / 2);

	HWND hWnd = ::CreateWindowExW(NULL                ,
								  windowClassName     ,
								  windowTitle         ,
								  WS_OVERLAPPEDWINDOW ,
								  windowX             ,
								  windowY             ,
								  windowWidth         ,
								  windowHeight        ,
								  NULL                ,
								  NULL                ,
								  hInst               ,
								  nullptr);

	assert(hWnd && "Failed to create window");
	
	return hWnd;
}

Window::Window(HINSTANCE hInst , const wchar_t* caption, WNDPROC Proc)
{
	RegisterWindowClass(hInst , caption , Proc);
	hWnd = CreateWindows(caption , hInst , L"OpenLight" , AppConfig::ClientWidth , AppConfig::ClientHeight);

	::GetWindowRect(hWnd , &WindowRect);
}

void Window::SetFullscreen(bool fullscreen)
{
	if (AppConfig::Fullscreen != fullscreen)
	{
		AppConfig::Fullscreen = fullscreen;

		if (AppConfig::Fullscreen)
		{
			// Store the current window dimensions so they can be stored
			// when switching out of fullscreen state
			::GetWindowRect(hWnd , &WindowRect);

			UINT windowStyle = WS_OVERLAPPEDWINDOW & (~WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			::SetWindowLongW(hWnd , GWL_STYLE , windowStyle);

			HMONITOR hMonitor = ::MonitorFromWindow(hWnd , MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(hMonitor , &monitorInfo);

			::SetWindowPos(hWnd                                                     ,
						   HWND_TOP                                                 ,
						   monitorInfo.rcMonitor.left                               ,
						   monitorInfo.rcMonitor.top                                ,
						   monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left ,
						   monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top ,
						   SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(hWnd , SW_MAXIMIZE);
		}
		else
		{
			::SetWindowLong(hWnd , GWL_STYLE , WS_OVERLAPPEDWINDOW);

			::SetWindowPos(hWnd                                ,
						   HWND_NOTOPMOST                      ,
						   WindowRect.left                     ,
						   WindowRect.top                      ,
						   WindowRect.right  - WindowRect.left ,
						   WindowRect.bottom - WindowRect.top  ,
						   SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(hWnd , SW_NORMAL);
		}
	}
}

HWND Window::GetHwnd()
{
	return hWnd;
}