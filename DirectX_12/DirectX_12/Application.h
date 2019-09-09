#pragma once
#include <windows.h>

class Application
{
private:
	Application();
	Application(const Application&) {};
	void operator=(const Application&) {};

public:
	~Application();
	static Application& GetInstance() {
		Application instance;
		return instance;
	}
	void InitWindow();
	void Initialize();
	void Run();
	void Terminate();
};

