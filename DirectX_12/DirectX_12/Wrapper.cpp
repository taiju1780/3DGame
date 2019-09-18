#include "Wrapper.h"
#include "Application.h"
#include <d3dcompiler.h>


//リンク
#pragma comment(lib,"d3d12.lib") 
#pragma comment(lib,"dxgi.lib") 
#pragma comment (lib,"d3dcompiler.lib")

using namespace DirectX;

struct Vertex {
	XMFLOAT3 pos; //座標
};

void Wrapper::InitSwapChain()
{
	auto& app = Application::GetInstance();
	auto wsize = app.GetWIndowSize();

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width					= wsize.w;								//画面幅
	swapchainDesc.Height				= wsize.h;								//画面高さ
	swapchainDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;			//データフォーマット
	swapchainDesc.Stereo				= false;								//ステレオにするかどうか
	swapchainDesc.SampleDesc.Count		= 1;									//マルチサンプリング数
	swapchainDesc.SampleDesc.Quality	= 0;									//イメージの範囲レベル(0～1)
	swapchainDesc.BufferCount			= 2;									//バッファ数
	swapchainDesc.BufferUsage			= DXGI_USAGE_BACK_BUFFER;				//サーフェスまたはリソースを出力レンダーターゲットとして使用
	swapchainDesc.Scaling				= DXGI_SCALING_STRETCH;					//サイズ合わせ
	swapchainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;		//表示方法
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

	//Queue
	D3D12_COMMAND_QUEUE_DESC cmdQdesc = {};
	cmdQdesc.Flags					= D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQdesc.NodeMask				= 0;
	cmdQdesc.Priority				= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQdesc.Type					= D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = _dev->CreateCommandQueue(&cmdQdesc, IID_PPV_ARGS(&_cmdQue));

	//Allocator
	//第一引数はQueueと一緒
	result = _dev->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&_cmdAllocator));

	//List
	result = _dev->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		_cmdAllocator,
		nullptr,
		IID_PPV_ARGS(&_cmdList));

	_cmdList->Close();
}

void Wrapper::InitDescriptorHeapRTV()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeeapDesc = {};
	descriptorHeeapDesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダーターゲットビュー
	descriptorHeeapDesc.NodeMask			= 0; 
	descriptorHeeapDesc.NumDescriptors		= 2; //表画面と裏画面分
	descriptorHeeapDesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	auto result = _dev->CreateDescriptorHeap(&descriptorHeeapDesc, IID_PPV_ARGS(&_rtvDescHeap));

	//先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescH = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();

	//デスクリプタ一個当たりのサイズを取得
	auto rtvHeapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	_swapchain->GetDesc(&swcDesc);
	_backBuffers.resize(swcDesc.BufferCount);

	for (int i = 0; i < _backBuffers.size(); i++) {
		auto result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&_backBuffers[i]));
		_dev->CreateRenderTargetView(_backBuffers[i], nullptr, rtvDescH);
		rtvDescH.ptr += rtvHeapSize;
	}

}

void Wrapper::InitVertices()
{
	Vertex vertices[] = {
		{{0.0f,0.0f,0.0f}},
		{{1.0f,0.0f,0.0f}},
		{{0.0f,-1.0f,0.0f}},
	};

	//リソースの初期化
	auto result = _dev->CreateCommittedResource(
	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_vertexBuffer));

	//マップしてメモリを確保
	Vertex* vBufferptr = nullptr;
	result = _vertexBuffer->Map(0, nullptr, (void**)&vBufferptr);
	memcpy(vBufferptr, vertices, sizeof(vertices));
	_vertexBuffer->Unmap(0, nullptr);

	//バッファービュー初期化
	_vbView.StrideInBytes = sizeof(Vertex);
	_vbView.SizeInBytes = sizeof(vertices);
	_vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
}

Wrapper::Wrapper(HINSTANCE h, HWND hwnd)
{
	//フィーチャーレベル
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
		//この中から NVIDIA の奴を探す
		for (auto adpt : adapters) {
			DXGI_ADAPTER_DESC adesc = {};
			adpt->GetDesc(&adesc);
			std::wstring strDesc = adesc.Description;
			if (strDesc.find(L"NVIDIA") != std::string::npos) {//NVIDIAアダプタを強制 
				adapter = adpt;
				break;
			}
		}
	}

	//デバイスの初期化
	D3D_FEATURE_LEVEL level;

	for (auto l : levels) {
		if (D3D12CreateDevice(nullptr, l, IID_PPV_ARGS(&_dev)) == S_OK) {
			level = l;
			break;
		}
	}

	InitCommand();

	InitFence();

	InitSwapChain();

	InitDescriptorHeapRTV();

	InitVertices();
}

Wrapper::~Wrapper()
{
}

void Wrapper::Update()
{
	auto heapStart = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	float clearColor[] = { 1.0f,0.0f,0.0f,1.0f };
	
	//コマンドのリセット
	auto result = _cmdAllocator->Reset();
	result = _cmdList->Reset(_cmdAllocator, nullptr);

	auto bbidx = _swapchain->GetCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescH = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto rtvHeapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	rtvDescH.ptr += (bbidx * rtvHeapSize);
	
	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);
	//クリア
	_cmdList->ClearRenderTargetView(heapStart,clearColor,0, nullptr);

	_cmdList->Close();

	ExecuteCmd();

	//待ち
	WaitExcute();

	_swapchain->Present(0, 0);
}

void Wrapper::ExecuteCmd()
{
	ID3D12CommandList* cmdLists[] = { _cmdList };
	_cmdQue->ExecuteCommandLists(1, cmdLists);
	//_cmdQue->Signal(_fence, ++_fenceValue);
}

void Wrapper::WaitExcute()
{
	_cmdQue->Signal(_fence, ++_fenceValue);
	while (_fence->GetCompletedValue() != _fenceValue);
}

void Wrapper::InitFence()
{
	_fenceValue = 0;
	auto result = _dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
}
