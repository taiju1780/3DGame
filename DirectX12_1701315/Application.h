#pragma once
#include <memory>
#include <windows.h>


struct Size {
	Size() {}
	Size(int inw, int inh) : w(inw), h(inh) {}
	int w;
	int h;
};

class Wrapper;

class Application
{
private:
	Application();
	Application(const Application&) {};
	void operator=(const Application&) {};

	HWND _hwnd;

	std::shared_ptr<Wrapper> _wrap;

	int _windowWidth;
	int _windowHeight;

	void InitWindow();

public:
	~Application();
	static Application& GetInstance() {
		static Application instance;
		return instance;
	}
	HWND GetWindowHandle() const;
	Size GetWIndowSize() const;
	void Initialize();
	void Run();
	void Terminate();
};

