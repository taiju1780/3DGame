#include "Wrapper.h"
#include "Application.h"
#include <vector>

//�����N
#pragma comment(lib,"d3d12.lib") 
#pragma comment(lib,"dxgi.lib") 


void Wrapper::InitSwapChain()
{
	auto& app = Application::GetInstance();
	auto wsize = app.GetWIndowSize();

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width					= wsize.w;								//��ʕ�
	swapchainDesc.Height				= wsize.h;								//��ʍ���
	swapchainDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;			//�f�[�^�t�H�[�}�b�g
	swapchainDesc.Stereo				= false;								//�X�e���I�ɂ��邩�ǂ���
	swapchainDesc.SampleDesc.Count		= 1;									//�}���`�T���v�����O��
	swapchainDesc.SampleDesc.Quality	= 0;									//�C���[�W�͈̔̓��x��(0�`1)
	swapchainDesc.BufferCount			= 2;									//�o�b�t�@��
	swapchainDesc.BufferUsage			= DXGI_USAGE_BACK_BUFFER;				//�T�[�t�F�X�܂��̓��\�[�X���o�̓����_�[�^�[�Q�b�g�Ƃ��Ďg�p
	swapchainDesc.Scaling				= DXGI_SCALING_STRETCH;					//�T�C�Y���킹
	swapchainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;		//�\�����@
	swapchainDesc.AlphaMode				= DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags					= false;

	HRESULT result = S_OK;
	result = factory->CreateSwapChainForHwnd(
		_cmdQue,
		app.GetWindowHandle(),
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)(&_swapchain)
	);
}

void Wrapper::InitCommand()
{
	HRESULT result = S_OK;

	D3D12_COMMAND_QUEUE_DESC cmdQdesc = {};
	cmdQdesc.Flags					= D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQdesc.NodeMask				= 0;
	cmdQdesc.Priority				= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQdesc.Type					= D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = _dev->CreateCommandQueue(&cmdQdesc, IID_PPV_ARGS(&_cmdQue));
}

void Wrapper::InitDescriptorHeapRTV()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeeapDesc = {};
	descriptorHeeapDesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //�����_�[�^�[�Q�b�g�r���[
	descriptorHeeapDesc.NodeMask			= 0; 
	descriptorHeeapDesc.NumDescriptors		= 2; //�\��ʂƗ���ʕ�
	descriptorHeeapDesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	auto result = _dev->CreateDescriptorHeap(&descriptorHeeapDesc, IID_PPV_ARGS(&_rtvDescHeap));

	auto rtvHeapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

Wrapper::Wrapper(HINSTANCE h, HWND hwnd)
{
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&factory));

	std::vector <IDXGIAdapter*> adapters;
	IDXGIAdapter* adapter = nullptr;
	for (int i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(adapter);
		//���̒����� NVIDIA �̓z��T��
		for (auto adpt : adapters) {
			DXGI_ADAPTER_DESC adesc = {};
			adpt->GetDesc(&adesc);
			std::wstring strDesc = adesc.Description;
			if (strDesc.find(L"NVIDIA") != std::string::npos) {//NVIDIA�A�_�v�^������ 
				adapter = adpt;
				break;
			}
		}
	}

	//�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL level;

	for (auto l : levels) {
		if (D3D12CreateDevice(nullptr, l, IID_PPV_ARGS(&_dev)) == S_OK) {
			level = l;
			break;
		}
	}

	InitCommand();

	InitSwapChain();
}


Wrapper::~Wrapper()
{
}

void Wrapper::Update()
{
}
