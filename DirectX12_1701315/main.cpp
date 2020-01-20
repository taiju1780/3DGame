#include "Application.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	auto& app = Application::GetInstance();
	app.Initialize();
	app.Run();
	app.Terminate();
	return 0;
}