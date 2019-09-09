#include "Application.h"
#include <wrl.h>

constexpr int WINDOW_WIDTH = 1200;
constexpr int WINDOW_HEIGHT = 800;

HWND _hwnd;

LRESULT WindowProcedure(HWND hwd, UINT msg, WPARAM wparam, LPARAM lparam) {

	if (msg == WM_DESTROY) {	//�E�C���h�E���j�����ꂽ��Ă΂�܂�
		PostQuitMessage(0);		//OS�ɑ΂���[�������̃A�v���͏I�����]�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwd, msg, wparam, lparam);	//���̏������s��
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
	// DirectX12�̃f�o�b�O���C���[��L���ɂ���
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
	w.lpfnWndProc	= (WNDPROC)WindowProcedure; //�R�[���o�b�N�֐��̎w��
	w.lpszClassName = "DX12_Project";			//�A�v���[�P�[�V�������̎w��
	w.hInstance		= GetModuleHandle(0);		//�n���h��
	RegisterClassEx(&w);						//�A�v���P�[�V�����N���X

	RECT wrc = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };//�E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//�E�B���h�E�̕ϐ��͊֐����g���ĕ␳����

	_hwnd = CreateWindow(
		w.lpszClassName,//�N���X���w��
		"DX12_1701315_�É���",//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,//�^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,//�\��X���W��OS�ɔC����
		CW_USEDEFAULT,//�\��Y���W��OS�ɔC����
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E��
		nullptr,//�e�E�B���h�E�n���h��
		nullptr,//���j���[�n���h��
		w.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr//�ǉ��p�����[�^
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
