#pragma once

class Window;
class Renderer;
class Scene;

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
	~App();

private:
	Window*	  window   = nullptr;
	Renderer* renderer = nullptr;
	Scene*	  scene    = nullptr;
};