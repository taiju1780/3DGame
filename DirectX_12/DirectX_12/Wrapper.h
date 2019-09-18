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

	///�X���b�v�`�F�C��������
	void InitSwapChain();

	///�R�}���h������
	void InitCommand();

	///RTV�p�̃f�X�N���v�^�[�q�[�v
	void InitDescriptorHeapRTV();

public:
	Wrapper(HINSTANCE h, HWND hwnd);
	~Wrapper();
	void Update();
};

