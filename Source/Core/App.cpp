#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cassert>
#include <chrono>

#include "AppConfig.h"
#include "Renderer.h"
#include "VegetationDemo/C1/C1Demo.h"
#include "VegetationDemo/C2/C2Demo.h"
#include "Window.h"
#include "App.h"

#include<windowsx.h>


#include "GUI/imgui.h"
#include "GUI/imgui_impl_win32.h"
#include "GUI/imgui_impl_dx12.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd , UINT message , WPARAM wParam , LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;
#if 1
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
		
			return true;
		case WM_MOUSEMOVE:
			//		std::cout << io.MousePos.x << "  " << io.MousePos.y << std::endl;

			return true;
		}
		break;
	}

	case WM_SIZE:
	{
		RECT clientRect = {};
		::GetClientRect(hWnd, &clientRect);

		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;

		App::GetInstance().GetRenderer()->Resize(width, height);
		break;
	}

	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	}
#endif

	return ::DefWindowProcW(hWnd , message , wParam , lParam);
}

void App::Init(HINSTANCE hInstance)
{
	window   = new Window(hInstance , L"OpenLight" , WndProc);
	renderer = new VegetationC2Demo;
//	renderer = new Renderer();
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
