#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

class Wrapper
{
private:
	ID3D12Device* _dev = nullptr;
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	ID3D12CommandQueue* _cmdQue = nullptr;
	IDXGIFactory6* factory = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;
	ID3D12Fence* _fence = nullptr;
	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;

	///スワップチェイン初期化
	void InitSwapChain();

	///コマンド初期化
	void InitCommand();

	///RTV用のデスクリプターヒープ
	void InitDescriptorHeapRTV();

public:
	Wrapper(HINSTANCE h, HWND hwnd);
	~Wrapper();
	void Update();
};

