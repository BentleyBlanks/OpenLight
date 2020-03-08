#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cassert>
#include <chrono>

#include "AppConfig.h"
#include "Renderer.h"
#include "TextureCubeRenderer.h"
#include "Window.h"
#include "App.h"

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
	renderer = new TextureCubRenderer;
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
