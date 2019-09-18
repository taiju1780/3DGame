#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>

class Wrapper
{
private:

	ID3D12Device* _dev = nullptr;

	IDXGIFactory6* factory = nullptr;

	//RTVデスクリプタヒープ関係
	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;
	std::vector<ID3D12Resource*> _backBuffers;

	///RTV用のデスクリプターヒープ初期化
	void InitDescriptorHeapRTV();

	//スワップチェイン
	///スワップチェイン初期化
	void InitSwapChain();
	IDXGISwapChain4* _swapchain = nullptr;

	//コマンド
	///コマンド初期化
	void InitCommand();
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	ID3D12CommandQueue* _cmdQue = nullptr;
	void ExecuteCmd();

	//フェンス
	///フェンス初期化
	void InitFence();
	UINT64 _fenceValue;//この値で待機するか執行するか判断する
	ID3D12Fence* _fence = nullptr; 
	///Queueが判断してこの関数で値を更新する
	void WaitExcute();

	//頂点データ
	ID3D12Resource* _vertexBuffer = nullptr;
	void InitVertices();
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};

public:
	Wrapper(HINSTANCE h, HWND hwnd);
	~Wrapper();
	void Update();
};

