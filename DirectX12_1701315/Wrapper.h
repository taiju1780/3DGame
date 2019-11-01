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

//ペラポリゴン(マルチパス用)
struct VertexTex {
	DirectX::XMFLOAT3 pos;//頂点座標
	DirectX::XMFLOAT2 uv;
};

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
	void InitVertices();
	ID3D12Resource* _vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};

	//シェーダ関連
	void InitShader();
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	//ルートシグネチャー
	void InitRootSignature();
	ID3D12RootSignature* _rootSignature = nullptr;
	
	//パイプライン
	void InitPipeline();
	ID3D12PipelineState* _pipeline = nullptr;

	//ビューポート、シザー
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	//四角形描画用のインデックス関係
	ID3D12Resource* _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _idxbView = {};

	//画像関係
	void InitTexture();
	ID3D12Resource* _texbuff;
	
	ID3D12DescriptorHeap* _texrtvHeap = nullptr;
	ID3D12DescriptorHeap* _texsrvHeap = nullptr;

	//カメラ
	std::shared_ptr<Camera> _camera;

	//model
	std::shared_ptr<PMDModel> _model;

	//pmxmodel
	std::shared_ptr<PMXModel> _pmxModel;

	//Floar
	std::shared_ptr<Floor> _floor;
	
	//頂点初期化
	void InitModelVertices();
	ID3D12Resource* _vertexModelBuffer = nullptr;
	ID3D12Resource* _indexModelBuffer = nullptr;

	//深度
	void InitDescriptorHeapDSV();
	ID3D12Resource* _dsvBuff;
	ID3D12DescriptorHeap* _dsvHeap = nullptr;
	ID3D12DescriptorHeap* _depthSrvHeap = nullptr;

	//ペラポリ
	D3D12_VERTEX_BUFFER_VIEW _1stvbView = {};
	ID3D12Resource* _peraBuff;

	D3D12_VIEWPORT _1stPathviewport;
	D3D12_RECT _1stPathscissorRect;


	//１パス目に使用するレンダリングバッファ
	ID3D12Resource* _1stPathBuff;
	ID3D12DescriptorHeap* _rtv1stDescHeap = nullptr;		//RTV(レンダーターゲット)デスクリプタヒープ
	ID3D12DescriptorHeap *_srv1stDescHeap = nullptr;		//その他(テクスチャ、定数)デスクリプタヒープ

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

