#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>

class Camera;
class PMDModel;
class PMXModel;
class Floor;

//�y���|���S��(�}���`�p�X�p)
struct VertexTex {
	DirectX::XMFLOAT3 pos;//���_���W
	DirectX::XMFLOAT2 uv;
};

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
	void InitVertices();
	ID3D12Resource* _vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};

	//�V�F�[�_�֘A
	void InitShader();
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	//���[�g�V�O�l�`���[
	void InitRootSignature();
	ID3D12RootSignature* _rootSignature = nullptr;
	
	//�p�C�v���C��
	void InitPipeline();
	ID3D12PipelineState* _pipeline = nullptr;

	//�r���[�|�[�g�A�V�U�[
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	//�l�p�`�`��p�̃C���f�b�N�X�֌W
	ID3D12Resource* _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _idxbView = {};

	//�摜�֌W
	void InitTexture();
	ID3D12Resource* _texbuff;
	
	ID3D12DescriptorHeap* _texrtvHeap = nullptr;
	ID3D12DescriptorHeap* _texsrvHeap = nullptr;

	//�J����
	std::shared_ptr<Camera> _camera;

	//model
	std::shared_ptr<PMDModel> _model;

	//pmxmodel
	std::shared_ptr<PMXModel> _pmxModel;

	//Floar
	std::shared_ptr<Floor> _floor;
	
	//���_������
	void InitModelVertices();
	ID3D12Resource* _vertexModelBuffer = nullptr;
	ID3D12Resource* _indexModelBuffer = nullptr;

	//�[�x
	void InitDescriptorHeapDSV();
	ID3D12Resource* _dsvBuff;
	ID3D12DescriptorHeap* _dsvHeap = nullptr;
	ID3D12DescriptorHeap* _depthSrvHeap = nullptr;

	//�y���|��
	D3D12_VERTEX_BUFFER_VIEW _1stvbView = {};
	ID3D12Resource* _peraBuff;

	D3D12_VIEWPORT _1stPathviewport;
	D3D12_RECT _1stPathscissorRect;


	//�P�p�X�ڂɎg�p���郌���_�����O�o�b�t�@
	ID3D12Resource* _1stPathBuff;
	ID3D12DescriptorHeap* _rtv1stDescHeap = nullptr;		//RTV(�����_�[�^�[�Q�b�g)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap *_srv1stDescHeap = nullptr;		//���̑�(�e�N�X�`���A�萔)�f�X�N���v�^�q�[�v

	ID3DBlob* peravertexShader = nullptr;
	ID3DBlob* perapixelShader = nullptr;

	ID3D12PipelineState* _perapipeline = nullptr;
	ID3D12RootSignature* _perarootsigunature = nullptr;

	void InitPath1stRTVSRV();
	void InitVerticesPera();
	void InitPath1stRootSignature();

public:
	Wrapper(HINSTANCE h, HWND hwnd);
	~Wrapper();
	void Update();
	void PeraUpdate();
};

