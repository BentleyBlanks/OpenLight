#pragma once
#include "Timer.h"
class Window;
class Renderer;
#define MacroGetDevice() App::GetInstance().GetRenderer()->GetDevice()
class App
{
public:
	void Init(HINSTANCE hInstance);
	void Update();

	// just for main thread
	static App& GetInstance();

	Renderer* GetRenderer() { return renderer; }
	Window*   GetWindow()	{ return window; }

private:
	App() {}
	App(const App&) {}
	App& operator= (const App&) { return *this; }

private:
	Window* window     = nullptr;
	Renderer* renderer = nullptr;
};