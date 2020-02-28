#include <Windows.h>
#include <shellapi.h>
#include "App.h"

int WINAPI WinMain(HINSTANCE hInstance , HINSTANCE hPreInstance , LPSTR lpCmdLine , int nShowCmd)
{
	// Windows 10 Creators Update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window
	// to achieve 100% scaling while still allowing non-client window content to
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	App::GetInstance().Init(hInstance);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg , NULL , 0 , 0 , PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessageA(&msg);
		}

		App::GetInstance().Update();
	}

	return 0;
}
