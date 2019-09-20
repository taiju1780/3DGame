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

	//RTV�f�X�N���v�^�q�[�v�֌W
	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;
	std::vector<ID3D12Resource*> _backBuffers;

	///RTV�p�̃f�X�N���v�^�[�q�[�v������
	void InitDescriptorHeapRTV();

	//�X���b�v�`�F�C��
	///�X���b�v�`�F�C��������
	void InitSwapChain();
	IDXGISwapChain4* _swapchain = nullptr;

	//�R�}���h
	///�R�}���h������
	void InitCommand();
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	ID3D12CommandQueue* _cmdQue = nullptr;
	void ExecuteCmd();

	//�t�F���X
	///�t�F���X������
	void InitFence();
	UINT64 _fenceValue;//���̒l�őҋ@���邩���s���邩���f����
	ID3D12Fence* _fence = nullptr; 
	///Queue�����f���Ă��̊֐��Œl���X�V����
	void WaitExcute();

	//���_�f�[�^
	ID3D12Resource* _vertexBuffer = nullptr;
	void InitVertices();
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};

	//�V�F�[�_�֘A
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;
	void InitShader();

	//���[�g�V�O�l�`���[
	ID3D12RootSignature* _rootSignature = nullptr;
	void InitRootSignature();

	//�p�C�v���C��
	ID3D12PipelineState* _pipeline = nullptr;
	void InitPipeline();

	//�r���[�|�[�g�A�V�U�[
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	//�l�p�`�`��p�̃C���f�b�N�X�֌W
	ID3D12Resource* _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _idxbView = {};

	//�摜�֌W
	void InitTexture();
	ID3D12Resource* _texbuff;

public:
	Wrapper(HINSTANCE h, HWND hwnd);
	~Wrapper();
	void Update();
};

