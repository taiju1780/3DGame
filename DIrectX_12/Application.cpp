#include "Application.h"
#include "Wrapper.h"
#include <wrl.h>

constexpr int WINDOW_WIDTH = 1200;
constexpr int WINDOW_HEIGHT = 800;

LRESULT WindowProcedure(HWND hwd, UINT msg, WPARAM wparam, LPARAM lparam) {

	if (msg == WM_DESTROY) {	//ウインドウが破棄されたら呼ばれます
		PostQuitMessage(0);		//OSに対して[もうこのアプリは終わるんや]と伝える
		return 0;
	}
	return DefWindowProc(hwd, msg, wparam, lparam);	//基底の処理を行う
}

Application::Application()
{
	_windowWidth = WINDOW_WIDTH;
	_windowHeight = WINDOW_HEIGHT;
}

Application::~Application()
{
}

HWND Application::GetWindowHandle() const
{
	return _hwnd;
}

Size Application::GetWIndowSize() const
{
	return Size(_windowWidth, _windowHeight);
}

void Application::InitWindow()
{
#if defined(_DEBUG)
	// DirectX12のデバッグレイヤーを有効にする
	{
		ID3D12Debug* debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			debugController->Release();
		}
	}
#endif

	WNDCLASSEX w	= {};
	w.cbSize		= sizeof(WNDCLASSEX);
	w.lpfnWndProc	= (WNDPROC)WindowProcedure; //コールバック関数の指定(どこに情報を返すか)
	w.lpszClassName = "DX12Project";			//アプリーケーション名の指定
	w.hInstance		= GetModuleHandle(0);		//ハンドル
	RegisterClassEx(&w);						//アプリケーションクラス

	RECT wrc = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };//ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//ウィンドウの変数は関数を使って補正する

	_hwnd = CreateWindow(
		w.lpszClassName,			//クラス名指定
		"DX12Project",				//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,		//タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,				//表示X座標はOSに任せる
		CW_USEDEFAULT,				//表示Y座標はOSに任せる
		_windowWidth,				//ウィンドウ幅
		_windowHeight,				//ウィンドウ高
		nullptr,					//親ウィンドウハンドル
		nullptr,					//メニューハンドル
		w.hInstance,				//呼び出しアプリケーションハンドル
		nullptr						//追加パラメータ
	);

	_wrap.reset(new Wrapper(w.hInstance, _hwnd));
}

void Application::Initialize()
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	InitWindow();
}

void Application::Run()
{
	ShowWindow(_hwnd, SW_SHOW);		//ウィンドウの表示

	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);		//仮想キー関連の交換
			DispatchMessage(&msg);		//処理されなかったメッセージをOSに投げ返す
		}

		if (msg.message == WM_QUIT) {
			break;						//ループ抜け
		}
		_wrap->Update();
	}
}

void Application::Terminate()
{
	CoUninitialize();
	UnregisterClass("DX12Project", GetModuleHandle(0));
}
