#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>

class Camera;
class PMDModel;

//struct PMDHeader {
//	char magic[3]; //"pmd"
//	float version;
//	char model_name[20];
//	char comment[256];
//};
//
//#pragma pack(1)
//struct PMDvertex {
//	float pos[3];				// x, y, z // 座標 
//	float normal_vec[3];		// nx, ny, nz // 法線ベクトル 
//	float uv[2];				// u, v // UV座標 // MMDは頂点UV 
//	unsigned short bone_num[2]; // ボーン番号1、番号2 // モデル変形(頂点移動)時に影響
//	unsigned char weight;
//	unsigned char edge;
//};
//#pragma pack()
//
//struct PMDMaterial
//{
//	float diffuse_color[3];		//rgb
//	float alpha;
//	float specular;
//	float specular_color[3];	//rgb
//	float mirror_color[3];		//rgb
//	unsigned char toon_index;	//toon.bmp
//	unsigned char edge_flag;	//輪郭、影
//	//ここまで４６バイト
//	unsigned int face_vert_count;	//面積点数
//	char texture_file_name[20];		//テクスチャファイル名
//};

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
	/*std::vector<PMDvertex> _verticesData;
	std::vector<unsigned short> _indexData;

	void InitModel(const char * filepath);*/

	//頂点初期化
	void InitModelVertices();
	ID3D12Resource* _vertexModelBuffer = nullptr;
	ID3D12Resource* _indexModelBuffer = nullptr;

	//深度
	void InitDescriptorHeapDSV();
	ID3D12Resource* _dsvBuff;
	ID3D12DescriptorHeap* _dsvHeap = nullptr;
	ID3D12DescriptorHeap* _depthSrvHeap = nullptr;

public:
	Wrapper(HINSTANCE h, HWND hwnd);
	~Wrapper();
	void Update();
};

