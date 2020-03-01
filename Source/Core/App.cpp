#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cassert>
#include <chrono>

#include "AppConfig.h"
#include "Renderer.h"
#include "Window.h"
#include "App.h"
#include "Scene.h"
#include "GameObject.h"

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
	scene	 = new Scene;
	window   = new Window(hInstance , L"OpenLight" , WndProc);
	renderer = new Renderer;
	renderer->Init(window->GetHwnd(), scene);

	::ShowWindow(window->GetHwnd() , SW_SHOW);

	{
		GameObject* o = new GameObject();
		Shader* shader = new Shader(renderer->GetDevice() , "../../Shader/HelloWorld.hlsl" , "vsmain" , "psmain");
		o->SetShader(shader);
		o->Load(renderer->GetDevice() , renderer->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY));
		scene->AddObject(o);
	}
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

App::~App()
{
	if (window)		delete window;
	if (renderer)	delete renderer;
	if (scene)		delete scene;
}
