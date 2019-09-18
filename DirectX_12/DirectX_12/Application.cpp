#include "Application.h"
#include "Wrapper.h"
#include <wrl.h>

constexpr int WINDOW_WIDTH = 1200;
constexpr int WINDOW_HEIGHT = 800;

LRESULT WindowProcedure(HWND hwd, UINT msg, WPARAM wparam, LPARAM lparam) {

	if (msg == WM_DESTROY) {	//�E�C���h�E���j�����ꂽ��Ă΂�܂�
		PostQuitMessage(0);		//OS�ɑ΂���[�������̃A�v���͏I�����]�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwd, msg, wparam, lparam);	//���̏������s��
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
	// DirectX12�̃f�o�b�O���C���[��L���ɂ���
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
	w.lpfnWndProc	= (WNDPROC)WindowProcedure; //�R�[���o�b�N�֐��̎w��(�ǂ��ɏ���Ԃ���)
	w.lpszClassName = "DX12Project";			//�A�v���[�P�[�V�������̎w��
	w.hInstance		= GetModuleHandle(0);		//�n���h��
	RegisterClassEx(&w);						//�A�v���P�[�V�����N���X

	RECT wrc = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };//�E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//�E�B���h�E�̕ϐ��͊֐����g���ĕ␳����

	_hwnd = CreateWindow(
		w.lpszClassName,			//�N���X���w��
		"DX12Project",				//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,		//�^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,				//�\��X���W��OS�ɔC����
		CW_USEDEFAULT,				//�\��Y���W��OS�ɔC����
		_windowWidth,				//�E�B���h�E��
		_windowHeight,				//�E�B���h�E��
		nullptr,					//�e�E�B���h�E�n���h��
		nullptr,					//���j���[�n���h��
		w.hInstance,				//�Ăяo���A�v���P�[�V�����n���h��
		nullptr						//�ǉ��p�����[�^
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
	ShowWindow(_hwnd, SW_SHOW);		//�E�B���h�E�̕\��

	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);		//���z�L�[�֘A�̌���
			DispatchMessage(&msg);		//��������Ȃ��������b�Z�[�W��OS�ɓ����Ԃ�
		}

		if (msg.message == WM_QUIT) {
			break;						//���[�v����
		}
		_wrap->Update();
	}
}

void Application::Terminate()
{
	CoUninitialize();
	UnregisterClass("DX12Project", GetModuleHandle(0));
}
