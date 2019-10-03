#include "Application.h"

int main() {
	auto& app = Application::GetInstance();
	app.Initialize();
	app.Run();
	app.Terminate();
	return 0;
}