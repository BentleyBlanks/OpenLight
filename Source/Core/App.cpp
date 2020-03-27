#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cassert>
#include <chrono>

#include "AppConfig.h"
#include "Renderer.h"
#include "TextureCubeRenderer.h"
#include "CubeRenderer.h"
#include "Window.h"
#include "App.h"
#include<windowsx.h>
extern int  gMouseX;
extern int  gMouseY;
extern int  gLastMouseX;
extern int  gLastMouseY;
extern bool gMouseDown;
extern bool gMouseMove;
extern HWND  gHwnd;
LRESULT CALLBACK WndProc(HWND hWnd , UINT message , WPARAM wParam , LPARAM lParam)
{
	switch (message)
	{
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

		switch (wParam)
		{
		case 'V':
			AppConfig::VSync = !AppConfig::VSync;
			break;

		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;

		case VK_RETURN:
			if (alt)
			{
		case VK_F11:
			App::GetInstance().GetWindow()->SetFullscreen(!AppConfig::Fullscreen);
			}
			break;
		case WM_LBUTTONDOWN:
			gLastMouseX = GET_X_LPARAM(lParam);
			gLastMouseY = GET_Y_LPARAM(lParam);
			return true;
		case WM_MOUSEMOVE:
			//		std::cout << io.MousePos.x << "  " << io.MousePos.y << std::endl;
			if (((wParam & MK_LBUTTON) != 0))
				gMouseMove = true;
			gMouseX = GET_X_LPARAM(lParam);
			gMouseY = GET_Y_LPARAM(lParam);
			return true;
		}
		break;
	}

	case WM_SIZE:
	{
		RECT clientRect = {};
		::GetClientRect(hWnd , &clientRect);

		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;

		App::GetInstance().GetRenderer()->Resize(width , height);
		break;
	}

	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	}

	return ::DefWindowProcW(hWnd , message , wParam , lParam);
}

void App::Init(HINSTANCE hInstance)
{
	window   = new Window(hInstance , L"OpenLight" , WndProc);
	renderer = new CubeRenderer;
	renderer->Init(window->GetHwnd());
	::ShowWindow(window->GetHwnd() , SW_SHOW);
}

void App::Update()
{
	if (!renderer)	return;

	renderer->Render();
}

App& App::GetInstance()
{
	static App app;
	return app;
}
