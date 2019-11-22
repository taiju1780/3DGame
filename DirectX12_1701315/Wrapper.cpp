#include "Wrapper.h"
#include "Application.h"
#include "Camera.h"
#include "PMDModel.h"
#include "PMXModel.h"
#include "Floor.h"
#include "Shadow.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <stdio.h>

//リンク
#pragma comment(lib,"d3d12.lib") 
#pragma comment(lib,"dxgi.lib") 
#pragma comment (lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;

unsigned int VertexSize = 38;

struct Vertex {
	XMFLOAT3 pos; //座標
	XMFLOAT2 uv; //uv
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
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダーターゲットビュー
	descriptorHeapDesc.NodeMask			= 0;
	descriptorHeapDesc.NumDescriptors	= 2; //表画面と裏画面分
	descriptorHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	auto result = _dev->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&_rtvDescHeap));

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

void Wrapper::InitShader()
{
	auto wsize = Application::GetInstance().GetWIndowSize();

	//マルチパス用シェーダ
	auto result = D3DCompileFromFile(L"MultiPath.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &peravertexShader, nullptr);
	result = D3DCompileFromFile(L"MultiPath.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &perapixelShader, nullptr);
	
	//マルチパス2用シェーダ
	result = D3DCompileFromFile(L"MultiPath2.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &pera2vertexShader, nullptr);
	result = D3DCompileFromFile(L"MultiPath2.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &pera2pixelShader, nullptr);

	//ビューポート設定
	_viewport.TopLeftX = 0;
	_viewport.TopLeftY = 0;
	_viewport.Width = wsize.w;
	_viewport.Height = wsize.h;
	_viewport.MaxDepth = 1.0f;
	_viewport.MinDepth = 0.0f;

	//シザー(切り取り)矩形
	_scissorRect.left = 0;
	_scissorRect.top = 0;
	_scissorRect.right = wsize.w;
	_scissorRect.bottom = wsize.h;
}

void Wrapper::InitPipeline()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//レンダーターゲット
	gpsDesc.NumRenderTargets	= 1;
	gpsDesc.RTVFormats[0]		= DXGI_FORMAT_R8G8B8A8_UNORM;

	//ラスタライザ
	gpsDesc.RasterizerState				= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;
	gpsDesc.RasterizerState.FillMode	= D3D12_FILL_MODE_SOLID;

	//その他
	gpsDesc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask				= 0;
	gpsDesc.SampleDesc.Count		= 1;
	gpsDesc.SampleDesc.Quality		= 0;
	gpsDesc.SampleMask				= D3D12_COLOR_WRITE_ENABLE_ALL;
	gpsDesc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//1stPath
	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC Peralayouts[] = {
		{ "POSITION",
		0,
		DXGI_FORMAT_R32G32B32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0 },

		//uvレイアウト
		{ "TEXCOORD",
		0,
		DXGI_FORMAT_R32G32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0 },
	};

	//ルートシグネチャーと頂点レイアウト
	gpsDesc.pRootSignature = _perarootsigunature;
	gpsDesc.InputLayout.pInputElementDescs = Peralayouts;
	gpsDesc.InputLayout.NumElements = _countof(Peralayouts);

	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = false;

	//シェーダ系
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(peravertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(perapixelShader);

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_perapipeline));

	//2ndPath
	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC Pera2layouts[] = {
		{ 
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0 
		},

		//uvレイアウト
		{ 
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0 
		},
	};

	//ルートシグネチャーと頂点レイアウト
	gpsDesc.pRootSignature = _pera2rootsigunature;
	gpsDesc.InputLayout.pInputElementDescs = Pera2layouts;
	gpsDesc.InputLayout.NumElements = _countof(Pera2layouts);

	//シェーダ系
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(pera2vertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pera2pixelShader);

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pera2pipeline));
}

void Wrapper::InitTexture()
{
	Application& app = Application::GetInstance();

	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type					= D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask		= 1;
	heapprop.VisibleNodeMask		= 1;

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Alignment			= 0;
	texDesc.Width				= app.GetWIndowSize().w;
	texDesc.Height				= app.GetWIndowSize().h;
	texDesc.DepthOrArraySize	= 1;
	texDesc.MipLevels			= 1;
	texDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count	= 1;
	texDesc.SampleDesc.Quality	= 0;
	texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	float clearColor[] = { 0.5,0.5,0.5,1.f };

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth	= 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	clearValue.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	std::copy(std::begin(clearColor), std::end(clearColor), clearValue.Color);

	auto result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&_texbuff));

	result = _cmdAllocator->Reset();
	result = _cmdList->Reset(_cmdAllocator, nullptr);

	//バリアセット
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			_texbuff,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	//画像読み込み
	TexMetadata metadata;
	ScratchImage img;
	result = LoadFromWICFile(L"img/IMG_0164.png", WIC_FLAGS_NONE, &metadata, img);

	D3D12_RESOURCE_DESC resdesk;
	resdesk = _texbuff->GetDesc();
	D3D12_BOX box = {};
	box.left = 0;
	box.right = (resdesk.Width);
	box.top = 0;
	box.bottom = (resdesk.Height);
	box.front = 0;
	box.back = 1;

	result = _texbuff->WriteToSubresource(0,&box,img.GetPixels(),metadata.width * 4/*RGBA(4)*/,img.GetPixelsSize());

	//バリア閉じ
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_texbuff,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST));

	_cmdList->Close();

	ExecuteCmd();
	WaitExcute();

	//デスクリプターヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HeapDesc.NodeMask					= 0;
	HeapDesc.NumDescriptors				= 1;
	HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	//レンダーターゲットデスクリプターヒープ作成
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_texrtvHeap));

	//シェーダリソースビューデスクリプターヒープ作成
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_texsrvHeap));

	//レンダーターゲットビュー
	auto HeapDescrtvH = _texrtvHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateRenderTargetView(_texbuff, nullptr, HeapDescrtvH);

	//シェーダーリソースビュー
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format							= DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels				= 1;
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto HeapDescsrvH = _texsrvHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateShaderResourceView(_texbuff, &srvDesc, HeapDescsrvH);
}

void Wrapper::InitPath1stRTVSRV()
{
	Application& app = Application::GetInstance();

	//デスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HeapDesc.NodeMask					= 0;
	HeapDesc.NumDescriptors				= 2;
	HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//ディスクリプタの型(レンダーターゲットビュー)

	//rtvデスクリプタヒープ作成
	auto result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_rtv1stDescHeap));

	//srvデスクリプタヒープ作成
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_srv1stDescHeap));

	_1stPathBuffers.resize(2);

	//ヒーププロパティ
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask		= 1;
	heapprop.VisibleNodeMask		= 1;
	heapprop.Type					= D3D12_HEAP_TYPE_CUSTOM;

	//テクスチャデスク
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Alignment			= 0;										//先頭からなので0
	texDesc.DepthOrArraySize	= 1;										//リソースが2Dで配列でもないので１
	texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//何次元テクスチャか(TEXTURE2D)
	texDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;	//NONE
	texDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;				//R8G8B8A8
	texDesc.Width				= app.GetWIndowSize().w;					//テクスチャ幅
	texDesc.Height				= app.GetWIndowSize().h;					//テクスチャ高さ
	texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;				//決定できないのでUNKNOWN
	texDesc.MipLevels			= 0;										//ミップ使ってないので0
	texDesc.SampleDesc.Count	= 1;
	texDesc.SampleDesc.Quality	= 0;

	float clearColor[] = { 0.5, 0.5, 0.5,1.f };

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth	= 1.0f; //最大値１
	clearValue.DepthStencil.Stencil = 0;
	clearValue.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	std::copy(std::begin(clearColor), std::end(clearColor), clearValue.Color);

	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescrtvH = _rtv1stDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescSrvH = _srv1stDescHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < _1stPathBuffers.size(); ++i) {
		
		//リソース
		auto result = _dev->CreateCommittedResource(
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(&_1stPathBuffers[i]));

		//レンダーターゲットビュー作成
		_dev->CreateRenderTargetView(_1stPathBuffers[i], nullptr, HeapDescrtvH);
		HeapDescrtvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		//シェーダーリソースビュー作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format							= texDesc.Format;
		srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels				= 1;
		srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		_dev->CreateShaderResourceView(_1stPathBuffers[i], &srvDesc, HeapDescSrvH);
		HeapDescSrvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

void Wrapper::InitVerticesPera()
{
	VertexTex vertices[] = {
		XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1),//正面 
		XMFLOAT3(-1,1,0),XMFLOAT2(0,0),//正面 
		XMFLOAT3(1,-1,0),XMFLOAT2(1,1),//正面 
		XMFLOAT3(1,1,0),XMFLOAT2(1,0),//正面
	};

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),	//CPUからGPUへ転送する用
		D3D12_HEAP_FLAG_NONE,								//特別な指定なし
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),	//サイズ
		D3D12_RESOURCE_STATE_GENERIC_READ,					//よくわからない
		nullptr,											//nullptrでいい
		IID_PPV_ARGS(&_peraBuff));

	D3D12_RANGE range = { 0,0 };
	VertexTex* vBuffptr = nullptr;
	result = _peraBuff->Map(0, nullptr, (void**)&vBuffptr);
	memcpy(vBuffptr, vertices, sizeof(vertices));
	_peraBuff->Unmap(0, nullptr);

	_1stvbView.BufferLocation = _peraBuff->GetGPUVirtualAddress();
	_1stvbView.SizeInBytes = sizeof(vertices);						//データ全体のサイズ
	_1stvbView.StrideInBytes = sizeof(VertexTex);
}

void Wrapper::InitPath1stRootSignature()
{
	//サンプラを設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//絵が繰り返される(U方向)
	samplerDesc.AddressV					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//絵が繰り返される(V方向)
	samplerDesc.AddressW					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//絵が繰り返される(W方向)
	samplerDesc.BorderColor					= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;				//エッジの色(黒透明)
	samplerDesc.Filter						= D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;					//特別なフィルタを使用しない
	samplerDesc.MaxLOD						= D3D12_FLOAT32_MAX;										//MIPMAP上限なし
	samplerDesc.MinLOD						= 0.0;														//MIPMAP下限なし
	samplerDesc.MipLODBias					= 0.0f;														//MIPMAPのバイアス
	samplerDesc.ShaderRegister				= 0;														//使用するシェーダレジスタ(スロット)
	samplerDesc.ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;								//どのくらいのデータをシェーダに見せるか(全部)
	samplerDesc.RegisterSpace				= 0;														//0でおｋ
	samplerDesc.MaxAnisotropy				= 0;														//FilterがAnisotropyの時のみ有効
	samplerDesc.ComparisonFunc				= D3D12_COMPARISON_FUNC_NEVER;								//特に比較しない(ではなく常に否定)

	//ルートシグネチャーの生成
	ID3DBlob* rootSignatureBlob = nullptr;	//ルートシグネチャをつくるための材料 
	ID3DBlob* error = nullptr;	//エラー出た時の対処

	D3D12_DESCRIPTOR_RANGE descTblrange[2] = {};
	D3D12_ROOT_PARAMETER rootparam[2] = {};

	//t0
	descTblrange[0].NumDescriptors						= 1;
	descTblrange[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblrange[0].BaseShaderRegister					= 0;
	descTblrange[0].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//t1(depth)
	descTblrange[1].NumDescriptors						= 1;
	descTblrange[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblrange[1].BaseShaderRegister					= 1;
	descTblrange[1].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//デスクリプターテーブル設定
	rootparam[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;
	rootparam[0].DescriptorTable.NumDescriptorRanges	= 1;
	rootparam[0].DescriptorTable.pDescriptorRanges		= &descTblrange[0];

	rootparam[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;
	rootparam[1].DescriptorTable.NumDescriptorRanges	= 1;
	rootparam[1].DescriptorTable.pDescriptorRanges		= &descTblrange[1];

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags						= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pParameters					= rootparam;
	rsd.NumParameters				= 2;
	rsd.pStaticSamplers				= &samplerDesc;
	rsd.NumStaticSamplers			= 1;

	auto result = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&rootSignatureBlob,
		&error);

	result = _dev->CreateRootSignature(
		0,
		rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&_perarootsigunature));
}

void Wrapper::InitPath2ndRTVSRV()
{
	Application& app = Application::GetInstance();

	//ヒーププロパティ
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask		= 1;
	heapprop.VisibleNodeMask		= 1;
	heapprop.Type					= D3D12_HEAP_TYPE_CUSTOM;

	//テクスチャデスク
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Alignment			= 0;										//先頭からなので0
	texDesc.DepthOrArraySize	= 1;										//リソースが2Dで配列でもないので１
	texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//何次元テクスチャか(TEXTURE2D)
	texDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;	//NONE
	texDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;				//R8G8B8A8
	texDesc.Width				= app.GetWIndowSize().w;					//テクスチャ幅
	texDesc.Height				= app.GetWIndowSize().h;					//テクスチャ高さ
	texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;				//決定できないのでUNKNOWN
	texDesc.MipLevels			= 0;										//ミップ使ってないので0
	texDesc.SampleDesc.Count	= 1;
	texDesc.SampleDesc.Quality	= 0;

	float clearColor[] = { 0.5, 0.5, 0.5,1.f };

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth	= 1.0f; //最大値１
	clearValue.DepthStencil.Stencil = 0;
	clearValue.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	std::copy(std::begin(clearColor), std::end(clearColor), clearValue.Color);

	//リソース
	auto result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(&_2ndPathBuff));

	//デスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HeapDesc.NodeMask		= 0;
	HeapDesc.NumDescriptors = 1;
	HeapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//ディスクリプタの型(レンダーターゲットビュー)

	//rtvデスクリプタヒープ作成
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_rtv2ndDescHeap));

	//srvデスクリプタヒープ作成
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_srv2ndDescHeap));

	//レンダーターゲットビュー作成
	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescrtvH = _rtv2ndDescHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateRenderTargetView(_2ndPathBuff, nullptr, HeapDescrtvH);

	//シェーダーリソースビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format							= DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels				= 1;
	srvDesc.Texture2D.MostDetailedMip		= 0;
	srvDesc.Texture2D.PlaneSlice			= 0;
	srvDesc.Texture2D.ResourceMinLODClamp	= 0.0F;
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescSrvH = _srv2ndDescHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateShaderResourceView(_2ndPathBuff, &srvDesc, HeapDescSrvH);
}

void Wrapper::InitVertices2Pera()
{
	VertexTex vertices[] = {
		XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1),//正面 
		XMFLOAT3(-1,1,0),XMFLOAT2(0,0),//正面 
		XMFLOAT3(1,-1,0),XMFLOAT2(1,1),//正面 
		XMFLOAT3(1,1,0),XMFLOAT2(1,0),//正面
	};

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),	//CPUからGPUへ転送する用
		D3D12_HEAP_FLAG_NONE,								//特別な指定なし
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),	//サイズ
		D3D12_RESOURCE_STATE_GENERIC_READ,					//よくわからない
		nullptr,											//nullptrでいい
		IID_PPV_ARGS(&_pera2Buff));

	D3D12_RANGE range = { 0,0 };
	VertexTex* vBuffptr = nullptr;
	result = _pera2Buff->Map(0, nullptr, (void**)&vBuffptr);
	memcpy(vBuffptr, vertices, sizeof(vertices));
	_pera2Buff->Unmap(0, nullptr);

	_2ndvbView.BufferLocation = _pera2Buff->GetGPUVirtualAddress();
	_2ndvbView.SizeInBytes = sizeof(vertices);						//データ全体のサイズ
	_2ndvbView.StrideInBytes = sizeof(VertexTex);
}

void Wrapper::InitPath2ndRootSignature()
{
	//サンプラを設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//絵が繰り返される(U方向)
	samplerDesc.AddressV				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//絵が繰り返される(V方向)
	samplerDesc.AddressW				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//絵が繰り返される(W方向)
	samplerDesc.BorderColor				= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;				//エッジの色(黒透明)
	samplerDesc.Filter					= D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;					//特別なフィルタを使用しない
	samplerDesc.MaxLOD					= D3D12_FLOAT32_MAX;										//MIPMAP上限なし
	samplerDesc.MinLOD					= 0.0;														//MIPMAP下限なし
	samplerDesc.MipLODBias				= 0.0f;														//MIPMAPのバイアス
	samplerDesc.ShaderRegister			= 0;														//使用するシェーダレジスタ(スロット)
	samplerDesc.ShaderVisibility		= D3D12_SHADER_VISIBILITY_ALL;								//どのくらいのデータをシェーダに見せるか(全部)
	samplerDesc.RegisterSpace			= 0;														//0でおｋ
	samplerDesc.MaxAnisotropy			= 0;														//FilterがAnisotropyの時のみ有効
	samplerDesc.ComparisonFunc			= D3D12_COMPARISON_FUNC_NEVER;								//特に比較しない(ではなく常に否定)

	//ルートシグネチャーの生成
	ID3DBlob* rootSignatureBlob = nullptr;	//ルートシグネチャをつくるための材料 
	ID3DBlob* error = nullptr;	//エラー出た時の対処

	D3D12_DESCRIPTOR_RANGE descTblrange[2] = {};
	D3D12_ROOT_PARAMETER rootparam[2] = {};

	//t0
	descTblrange[0].NumDescriptors						= 1;
	descTblrange[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblrange[0].BaseShaderRegister					= 0;
	descTblrange[0].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//t1(depth)
	descTblrange[1].NumDescriptors						= 1;
	descTblrange[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblrange[1].BaseShaderRegister					= 1;
	descTblrange[1].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//デスクリプターテーブル設定
	rootparam[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;
	rootparam[0].DescriptorTable.NumDescriptorRanges	= 1;
	rootparam[0].DescriptorTable.pDescriptorRanges		= &descTblrange[0];

	rootparam[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;
	rootparam[1].DescriptorTable.NumDescriptorRanges	= 1;
	rootparam[1].DescriptorTable.pDescriptorRanges		= &descTblrange[1];

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags					= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pParameters				= rootparam;
	rsd.NumParameters			= 2;
	rsd.pStaticSamplers			= &samplerDesc;
	rsd.NumStaticSamplers		= 1;

	auto result = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&rootSignatureBlob,
		&error);

	result = _dev->CreateRootSignature(
		0,
		rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&_pera2rootsigunature));
}

void Wrapper::InitDescriptorHeapDSV()
{
	auto &app = Application::GetInstance();

	//デスクリプターヒープ
	D3D12_DESCRIPTOR_HEAP_DESC _dsvDesc = {};
	_dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	_dsvDesc.NodeMask = 0;
	_dsvDesc.NumDescriptors = 1;
	_dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	auto result = _dev->CreateDescriptorHeap(&_dsvDesc, IID_PPV_ARGS(&_dsvHeap));

	//深度バッファ
	D3D12_HEAP_PROPERTIES heappropDsv = {};
	heappropDsv.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heappropDsv.CreationNodeMask = 0;
	heappropDsv.VisibleNodeMask = 0;
	heappropDsv.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heappropDsv.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC dsvDesc = {};
	dsvDesc.Alignment = 0;
	dsvDesc.Width = app.GetWIndowSize().w;
	dsvDesc.Height = app.GetWIndowSize().h;
	dsvDesc.DepthOrArraySize = 1;
	dsvDesc.MipLevels = 0;
	dsvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	dsvDesc.SampleDesc.Count = 1;
	dsvDesc.SampleDesc.Quality = 0;
	dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	//クリアバリュー
	D3D12_CLEAR_VALUE clearValue;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;

	result = _dev->CreateCommittedResource(
		&heappropDsv,
		D3D12_HEAP_FLAG_NONE,
		&dsvDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&_dsvBuff));

	//深度バッファービュー
	D3D12_DEPTH_STENCIL_VIEW_DESC _dsvVDesc = {};
	_dsvVDesc.Format = DXGI_FORMAT_D32_FLOAT;
	_dsvVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	_dsvVDesc.Texture2D.MipSlice = 0;
	_dsvVDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView(_dsvBuff, &_dsvVDesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Wrapper::DrawLightView()
{
	_cmdList->SetPipelineState(_shadow->GetShadowPipeline());

	_cmdList->SetGraphicsRootSignature(_shadow->GetShadowRootSignature());

	_viewport.Width = _shadow->Getbuff()->GetDesc().Width;
	_viewport.Height = _shadow->Getbuff()->GetDesc().Height;

	_scissorRect.right = _shadow->Getbuff()->GetDesc().Width;
	_scissorRect.bottom = _shadow->Getbuff()->GetDesc().Height;

	//ビューポート、シザー
	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	//バリアー
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_shadow->Getbuff(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE));

	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(0, nullptr, false, &_shadow->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart());

	//深度バッファのクリア
	auto _sdsv = _shadow->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
	_cmdList->ClearDepthStencilView(_sdsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	//ヒープセット
	_cmdList->SetDescriptorHeaps(1, &_camera->GetrgstDescHeap());
	_cmdList->SetGraphicsRootDescriptorTable(0, _camera->GetrgstDescHeap()->GetGPUDescriptorHandleForHeapStart());

	//トポロジセット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//バッファービューのセット
	for (auto &pmxmodel : _pmxModels) {
		_cmdList->IASetVertexBuffers(0, 1, &pmxmodel->GetvView());
		_cmdList->IASetIndexBuffer(&pmxmodel->GetidxbView());

		//ボーンヒープ
		_cmdList->SetDescriptorHeaps(1, &pmxmodel->GetBoneHeap());
		_cmdList->SetGraphicsRootDescriptorTable(1, pmxmodel->GetBoneHeap()->GetGPUDescriptorHandleForHeapStart());

		//モデル表示
		unsigned int offset = 0;

		for (auto m : pmxmodel->GetMatData()) {
			_cmdList->DrawIndexedInstanced(m.face_vert_cnt, 1, offset, 0, 0);
			offset += m.face_vert_cnt;
		}
	}

	//バリアー
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_shadow->Getbuff(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		));
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

	InitDescriptorHeapDSV();

	InitPath1stRTVSRV();

	InitPath2ndRTVSRV();

	_camera.reset(new Camera(_dev));

	//PMDModel
	//////////////////////////////////////////////////////////

	//const char* cfilepath = ("Model/初音ミク.pmd");
	//const char* cfilepath = ("Model/巡音ルカ.pmd");
	//const char* cfilepath = ("Model/初音ミクmetal.pmd");
	const char* cfilepath = ("Model/初音ミクXS改変雪桜-1.1/mikuXS桜ミク.pmd");
	//const char* cfilepath = ("Model/hibiki/我那覇響v1.pmd");
	//const char* cfilepath = ("Model/hibari/雲雀Ver1.10.pmd");
	//const char* cfilepath = ("Model/博麗霊夢/reimu_F01.pmd");

	//PMXModel
	/////////////////////////////////////////////////////////
	//const char* xfilepath = ("PMXModel/m_GUMI_V3_201306/GUMIβ_V3.pmx");
	//const char* xfilepath = ("PMXModel/ちびルーミア/ちびルーミア.pmx");
	//const char* xfilepath = ("PMXModel/YYB式改変初音ミク(PRT.B-Cos)/YYB式改変初音ミク(PRT.B-Cos).pmx");
	//const char* xfilepath = ("PMXModel/レムとラム Ver. 1.02/Rem Ver. 1.02.pmx");
	//const char* xfilepath = ("PMXModel/レムとラム Ver. 1.02/Ram.pmx");
	const char* xfilepath = ("PMXModel/na_2b_0407h/na_2b_0407.pmx");
	//const char* x2filepath = ("PMXModel/ちびルーミア/ちびルーミア.pmx");
	const char* x2filepath = ("PMXModel/TB/TB.pmx");

	//モーション(アクション)
	//const char* mfilepath = ("Motion/pose.vmd");
	//const char* mfilepath = ("Motion/swing2.vmd");
	//const char* mfilepath = ("Motion/charge.vmd.vmd");
	//const char* mfilepath = ("Motion/first.vmd");

	//モーション(ダンス)
	//const char* mfilepath = ("Motion/45秒GUMI.vmd");
	const char* mfilepath = ("Motion/ストロボナイツモーション.vmd");
	//const char* m2filepath = ("Motion/えれくとりっくえんじぇぅ.vmd");
	//const char* mfilepath = ("Motion/ヤゴコロダンス.vmd");
	const char* m2filepath = ("Motion/45秒MIKU.vmd");

	_model.reset(new PMDModel(cfilepath,_dev));

	//_pmxModels.emplace_back(std::make_shared<PMXModel>(x2filepath, _dev));
	_pmxModels.emplace_back(std::make_shared<PMXModel>(xfilepath, _dev));

	_pmxModels[0]->InitMotion(mfilepath, _dev);
	//_pmxModels[1]->InitMotion(m2filepath, _dev);

	for (auto &xmodel : _pmxModels) {

		xmodel->InitModel(_dev);

		xmodel->InitBone(_dev);
	}
	
	_floor.reset(new Floor(_dev));

	_shadow.reset(new Shadow(_dev));

	InitVerticesPera();

	InitVertices2Pera();

	InitShader();

	InitTexture();

	InitPath1stRootSignature();

	InitPath2ndRootSignature();

	_floor->InitRootSignature(_dev);

	_shadow->InitRootSignature(_dev);

	InitPipeline();

	_floor->InitPiplineState(_dev);

	_shadow->InitPipline(_dev);
}

Wrapper::~Wrapper()
{
}

void Wrapper::Update()
{
	unsigned char keyState[256];
	_camera->CameraUpdate(keyState);

	for (auto &xmodel : _pmxModels) {
		xmodel->Update();
	}

	auto heapStart = _rtv1stDescHeap->GetCPUDescriptorHandleForHeapStart();

	float clearColor[] = { 0,0,0.5f,1.0f };
	
	auto bbidx = _swapchain->GetCurrentBackBufferIndex();
	auto rtvHeapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//コマンドのリセット
	auto result = _cmdAllocator->Reset();
	result = _cmdList->Reset(_cmdAllocator, nullptr);

	DrawLightView();

	for (int i = 0; i < _1stPathBuffers.size(); ++i) {
		//バリアセット
		_cmdList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
			_1stPathBuffers[i],
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(_1stPathBuffers.size(), &heapStart, true, &_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	

	for (int i = 0; i < _1stPathBuffers.size(); ++i) {
		//クリアレンダーターゲット
		_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);
		heapStart.ptr += rtvHeapSize;
	}

	//深度バッファをクリア
	_cmdList->ClearDepthStencilView(_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	for (auto &xmodel : _pmxModels) {
		xmodel->Draw(_dev, _cmdList, _camera, _rtv1stDescHeap);
	}

	//床
	_cmdList->SetPipelineState(_floor->_GetPipeline());
	_cmdList->SetGraphicsRootSignature(_floor->GetRootSignature());

	_cmdList->SetDescriptorHeaps(1, &_camera->GetrgstDescHeap());
	_cmdList->SetGraphicsRootDescriptorTable(0, _camera->GetrgstDescHeap()->GetGPUDescriptorHandleForHeapStart());

	//shadow用
	_cmdList->SetDescriptorHeaps(1, &_shadow->GetSrvHeap());
	_cmdList->SetGraphicsRootDescriptorTable(1, _shadow->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_floor->GetView());
	_cmdList->DrawInstanced(4, 1, 0, 0);

	for (int i = 0; i < _1stPathBuffers.size(); ++i) {
		//バリア閉じ
		_cmdList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
			_1stPathBuffers[i],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	_cmdList->Close();

	ExecuteCmd();

	//待ち
	WaitExcute();

	PeraUpdate();

	Pera2Update();

	_swapchain->Present(0, 0);
}

void Wrapper::PeraUpdate()
{
	auto result = _cmdAllocator->Reset();							//アロケーターのリセット
	result = _cmdList->Reset(_cmdAllocator, _perapipeline);		//コマンドリストのリセット

	//現在のバックバッファのインデックス
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			_2ndPathBuff,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	//マルチパス用のルートシグネチャー
	_cmdList->SetGraphicsRootSignature(_perarootsigunature);

	//ビューポート、シザー
	_viewport.Width = _1stPathBuffers[0]->GetDesc().Width;
	_viewport.Height = _1stPathBuffers[0]->GetDesc().Height;

	_scissorRect.right = _1stPathBuffers[0]->GetDesc().Width;
	_scissorRect.bottom = _1stPathBuffers[0]->GetDesc().Height;

	auto desc = _1stPathBuffers[1]->GetDesc();

	D3D12_VIEWPORT vp = _viewport;
	D3D12_RECT sr = _scissorRect;
	vp.Height = desc.Height / 2;
	vp.Width = desc.Width;
	sr.top = 0;
	sr.left = 0;
	sr.right = vp.Width;
	sr.bottom = vp.Height;

	//レンダーターゲット指定
	auto rtv = _rtv2ndDescHeap->GetCPUDescriptorHandleForHeapStart();

	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(1, &rtv, false, nullptr);

	//クリアカラー設定
	float clearColor[] = { 0.5, 0.5, 0.5,1.f };

	//レンダーターゲットのクリア
	_cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

	//ペラポリ
	_cmdList->SetDescriptorHeaps(1, &_srv1stDescHeap);
	_cmdList->SetGraphicsRootDescriptorTable(0, _srv1stDescHeap->GetGPUDescriptorHandleForHeapStart());

	_cmdList->SetDescriptorHeaps(1, &_shadow->GetSrvHeap());
	_cmdList->SetGraphicsRootDescriptorTable(1, _shadow->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//バッファービューのセット
	_cmdList->IASetVertexBuffers(0, 1, &_1stvbView);

	for (int i = 0; i < 4; ++i) {
		_cmdList->RSSetViewports(1, &vp);
		_cmdList->RSSetScissorRects(1, &sr);

		_cmdList->DrawInstanced(4, 1, 0, 0);

		sr.top += vp.Height;
		vp.TopLeftX = 0;
		vp.TopLeftY = sr.top;
		vp.Width /= 2;
		vp.Height /= 2;
		sr.bottom = sr.top + vp.Height;
	}

	//バリアー
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			_2ndPathBuff,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));

	_cmdList->Close();		//コマンドのクローズ

	ExecuteCmd();
	WaitExcute();
}

void Wrapper::Pera2Update()
{
	auto result = _cmdAllocator->Reset();							//アロケーターのリセット
	result = _cmdList->Reset(_cmdAllocator, _pera2pipeline);		//コマンドリストのリセット

	//現在のバックバッファのインデックス
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	//マルチパス用のルートシグネチャー
	_cmdList->SetGraphicsRootSignature(_pera2rootsigunature);

	//ビューポート、シザー
	_viewport.Width = _2ndPathBuff->GetDesc().Width;
	_viewport.Height = _2ndPathBuff->GetDesc().Height;

	_scissorRect.right = _2ndPathBuff->GetDesc().Width;
	_scissorRect.bottom = _2ndPathBuff->GetDesc().Height;

	//ビューポート、シザー
	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	//レンダーターゲット指定
	auto rtv = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(1, &rtv, false, nullptr);

	//クリアカラー設定
	float clearColor[] = { 0.5, 0.5, 0.5,1.f };

	//レンダーターゲットのクリア
	_cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

	//ペラポリ
	_cmdList->SetDescriptorHeaps(1, &_srv2ndDescHeap);
	_cmdList->SetGraphicsRootDescriptorTable(0, _srv2ndDescHeap->GetGPUDescriptorHandleForHeapStart());

	_cmdList->SetDescriptorHeaps(1, &_shadow->GetSrvHeap());
	_cmdList->SetGraphicsRootDescriptorTable(1, _shadow->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//バッファービューのセット
	_cmdList->IASetVertexBuffers(0, 1, &_2ndvbView);

	_cmdList->DrawInstanced(4, 1, 0, 0);

	//バリアー
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	_cmdList->Close();		//コマンドのクローズ

	ExecuteCmd();
	WaitExcute();
}

void Wrapper::ExecuteCmd()
{
	ID3D12CommandList* _cmdLists[] = { _cmdList };
	_cmdQue->ExecuteCommandLists(1, _cmdLists);
	_cmdQue->Signal(_fence, ++_fenceValue);
}

void Wrapper::WaitExcute()
{
	while (_fence->GetCompletedValue() < _fenceValue);
}

void Wrapper::InitFence()
{
	_fenceValue = 0;
	auto result = _dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
}
