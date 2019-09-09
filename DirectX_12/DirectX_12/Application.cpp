#include "Application.h"
#include <wrl.h>

constexpr int WINDOW_WIDTH = 1200;
constexpr int WINDOW_HEIGHT = 800;

HWND _hwnd;

LRESULT WindowProcedure(HWND hwd, UINT msg, WPARAM wparam, LPARAM lparam) {

	if (msg == WM_DESTROY) {	//ウインドウが破棄されたら呼ばれます
		PostQuitMessage(0);		//OSに対して[もうこのアプリは終わるんや]と伝える
		return 0;
	}
	return DefWindowProc(hwd, msg, wparam, lparam);	//基底の処理を行う
}

Application::Application()
{
}

Application::~Application()
{
}

void Application::InitWindow()
{
#if defined(_DEBUG)
	// DirectX12のデバッグレイヤーを有効にする
	{
		/*ID3D12Debug* debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			debugController->Release();
		}*/
	}
#endif

	WNDCLASSEX w	= {};
	w.cbSize		= sizeof(WNDCLASSEX);
	w.lpfnWndProc	= (WNDPROC)WindowProcedure; //コールバック関数の指定
	w.lpszClassName = "DX12_Project";			//アプリーケーション名の指定
	w.hInstance		= GetModuleHandle(0);		//ハンドル
	RegisterClassEx(&w);						//アプリケーションクラス

	RECT wrc = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };//ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//ウィンドウの変数は関数を使って補正する

	_hwnd = CreateWindow(
		w.lpszClassName,//クラス名指定
		"DX12_1701315_古賀大樹",//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,//表示X座標はOSに任せる
		CW_USEDEFAULT,//表示Y座標はOSに任せる
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		w.hInstance,//呼び出しアプリケーションハンドル
		nullptr//追加パラメータ
	);
}

void Application::Initialize()
{
}

void Application::Run()
{
}

void Application::Terminate()
{
}
