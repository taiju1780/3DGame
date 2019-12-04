#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

class Camera;
class PMDModel;
class PMXModel;
class Floor;
class Shadow;

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

	////�V�F�[�_�֘A
	void InitShader();
	
	////�p�C�v���C��
	void InitPipeline();

	//�r���[�|�[�g�A�V�U�[
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	//�摜�֌W
	void InitTexture();
	ID3D12Resource* _texbuff;
	
	ID3D12DescriptorHeap* _texrtvHeap = nullptr;
	ID3D12DescriptorHeap* _texsrvHeap = nullptr;

	//�[�x
	void InitDescriptorHeapDSV();
	ID3D12Resource* _dsvBuff;
	ID3D12DescriptorHeap* _dsvHeap = nullptr;
	ID3D12DescriptorHeap* _depthSrvHeap = nullptr;

	//�J����
	std::shared_ptr<Camera> _camera;

	//pmdmodel
	std::shared_ptr<PMDModel> _model;

	//pmxmodel
	std::shared_ptr<PMXModel> _pmxModel;
	std::vector<std::shared_ptr<PMXModel>> _pmxModels;

	//Floar
	std::shared_ptr<Floor> _floor;

	//shadow
	std::shared_ptr<Shadow> _shadow;

	//�y���|��
	D3D12_VERTEX_BUFFER_VIEW _1stvbView = {};
	ID3D12Resource* _peraBuff;

	//�P�p�X�ڂɎg�p���郌���_�����O�o�b�t�@
	std::vector<ID3D12Resource*> _1stPathBuffers;
	ID3D12DescriptorHeap* _rtv1stDescHeap = nullptr;		//RTV(�����_�[�^�[�Q�b�g)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _srv1stDescHeap = nullptr;		//���̑�(�e�N�X�`���A�萔)�f�X�N���v�^�q�[�v

	ID3DBlob* peravertexShader = nullptr;
	ID3DBlob* perapixelShader = nullptr;

	ID3D12PipelineState* _perapipeline = nullptr;
	ID3D12RootSignature* _perarootsigunature = nullptr;

	void InitPath1stRTVSRV();
	void InitVerticesPera();
	void InitPath1stRootSignature();

	//�y��2�|��
	D3D12_VERTEX_BUFFER_VIEW _2ndvbView = {};
	ID3D12Resource* _pera2Buff;

	//2�p�X�ڂɎg�p���郌���_�����O�o�b�t�@
	ID3D12Resource* _2ndPathBuff;
	ID3D12DescriptorHeap* _rtv2ndDescHeap = nullptr;		//RTV(�����_�[�^�[�Q�b�g)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap *_srv2ndDescHeap = nullptr;		//���̑�(�e�N�X�`���A�萔)�f�X�N���v�^�q�[�v

	ID3DBlob* pera2vertexShader = nullptr;
	ID3DBlob* pera2pixelShader = nullptr;

	ID3D12PipelineState* _pera2pipeline = nullptr;
	ID3D12RootSignature* _pera2rootsigunature = nullptr;

	void InitPath2ndRTVSRV();
	void InitVertices2Pera();
	void InitPath2ndRootSignature();

	//�e
	void DrawLightView();

	//�u���[��
	//bloom
	/*ID3D12DescriptorHeap* _bloomrtv;
	ID3D12DescriptorHeap* _bloomsrv;
	std::vector<ID3D12Resource*> _bloomBuffers;*/
	//void InitBloomRTVSRV(ID3D12Device * _dev);

	//effekseer
	void InitEffekseer();
	Effekseer::Manager* efkManager;
	Effekseer::Effect* effect;
	EffekseerRenderer::Renderer* efkRenderer;
	EffekseerRenderer::SingleFrameMemoryPool* efkMemoryPool;
	EffekseerRenderer::CommandList* efkCmdList;
	Effekseer::Handle efkHandle;

	//imgui
	void InitIMGUI(HWND hwnd);
	ID3D12DescriptorHeap* imguiHeap;

public:
	Wrapper(HINSTANCE h, HWND hwnd);
	~Wrapper();
	void Update();
	void PeraUpdate();
	void Pera2Update();
};

