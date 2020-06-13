#pragma once

#include <Windows.h>

class App;
class Window
{
public:
	friend App;
	Window(HINSTANCE hInst , const wchar_t* caption , WNDPROC Proc);

	void SetFullscreen(bool fullscreen);
	HWND GetHwnd();

private:
	HWND hWnd;
	RECT WindowRect;
};