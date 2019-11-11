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

void Wrapper::InitVertices()
{
	Vertex vertices[] = {
		{{-0.8f,0.8f,0.0f},{0,0}},
		{{0.8f,0.8f,0.0f},{1,0}},
		{{-0.8f,-0.8f,0.0f},{0,1}},
		{{0.8f,-0.8f,0.0f},{1,1}},
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

	//多頂点用インデックス
	std::vector<unsigned short> indices = { 0,2,1,2,3,1 };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer));

	//マップ
	short* idxBufptr = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxBufptr);
	std::copy(indices.begin(), indices.end(), idxBufptr);
	_indexBuffer->Unmap(0, nullptr);

	//バッファビューの設定
	_idxbView.BufferLocation	= _indexBuffer->GetGPUVirtualAddress();
	_idxbView.Format			= DXGI_FORMAT_R16_UINT;
	_idxbView.SizeInBytes		= indices.size() * sizeof(indices[0]);
}

void Wrapper::InitShader()
{
	auto wsize = Application::GetInstance().GetWIndowSize();

	//モデル用シェーダ
	auto result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &vertexShader, nullptr);
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &pixelShader, nullptr);

	//マルチパス用シェーダ
	result = D3DCompileFromFile(L"MultiPath.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &peravertexShader, nullptr);
	result = D3DCompileFromFile(L"MultiPath.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &perapixelShader, nullptr);

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

void Wrapper::InitRootSignature()
{
	//サンプラ
	D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].AddressU					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressV					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressW					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].BorderColor				= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//エッジの色
	samplerDesc[0].Filter					= D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;//特別なフィルタを使用しない
	samplerDesc[0].MaxLOD					= D3D12_FLOAT32_MAX;
	samplerDesc[0].MinLOD					= 0.0f;
	samplerDesc[0].MipLODBias				= 0.0f;
	samplerDesc[0].ShaderRegister			= 0;
	samplerDesc[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;//どのくらいシェーダに見せるか
	samplerDesc[0].RegisterSpace			= 0;
	samplerDesc[0].MaxAnisotropy			= 0;
	samplerDesc[0].ComparisonFunc			= D3D12_COMPARISON_FUNC_NEVER;

	samplerDesc[1]							= samplerDesc[0];
	samplerDesc[1].AddressU					= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressV					= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressW					= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].ShaderRegister			= 1;
	
	ID3DBlob* signature = nullptr;//ID3D12Blob=メモリオブジェクト
	ID3DBlob* error = nullptr;

	D3D12_DESCRIPTOR_RANGE descTblRange[4] = {};
	D3D12_ROOT_PARAMETER rootParam[3] = {};

	//デスクリプタレンジの設定
	//座標変換定数バッファ
	descTblRange[0].BaseShaderRegister					= 0;//レジスタ番号
	descTblRange[0].NumDescriptors						= 1;
	descTblRange[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[0].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//マテリアル用定数バッファ
	descTblRange[1].BaseShaderRegister					= 1;//レジスタ番号
	descTblRange[1].NumDescriptors						= 1;
	descTblRange[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[1].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//テクスチャ用バッファ(SRV){基本、sph、spa、toon}
	descTblRange[2].BaseShaderRegister					= 0;//レジスタ番号
	descTblRange[2].NumDescriptors						= 4;
	descTblRange[2].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblRange[2].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//ボーン用定数バッファ
	descTblRange[3].BaseShaderRegister					= 2;//レジスタ番号
	descTblRange[3].NumDescriptors						= 1;
	descTblRange[3].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[3].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//paramの設定
	//座標変換
	rootParam[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	rootParam[0].DescriptorTable.NumDescriptorRanges	= 1;
	rootParam[0].DescriptorTable.pDescriptorRanges		= &descTblRange[0];
	
	//マテリアル＋テクスチャ
	rootParam[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	rootParam[1].DescriptorTable.NumDescriptorRanges	= 2;
	rootParam[1].DescriptorTable.pDescriptorRanges		= &descTblRange[1];

	//ボーン用
	rootParam[2].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].ShaderVisibility						= D3D12_SHADER_VISIBILITY_VERTEX;
	rootParam[2].DescriptorTable.NumDescriptorRanges	= 1;
	rootParam[2].DescriptorTable.pDescriptorRanges		= &descTblRange[3];
	
	//ルートシグネチャ
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags				= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pParameters			= rootParam;
	rsd.pStaticSamplers		= samplerDesc;
	rsd.NumParameters		= 3;
	rsd.NumStaticSamplers	= 2;

	auto result = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error
	);

	result = _dev->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_rootSignature));
}

void Wrapper::InitPipeline()
{
	D3D12_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },

		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},	

		//追加UV
		//{"ADDUV",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		//{"ADDUV",1,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		//{"ADDUV",2,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		//{"ADDUV",3,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		////ウェイトタイプ
		//{"WEIGHT_TYPE",0,DXGI_FORMAT_R32_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },

		////ボーンイデックス
		//{"BONEINDEX",0,DXGI_FORMAT_R32G32B32A32_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		////ウェイト
		//{"WEIGHT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};


	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature					= _rootSignature;
	gpsDesc.InputLayout.pInputElementDescs	= layout;
	gpsDesc.InputLayout.NumElements			= _countof(layout);

	//シェーダ
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);

	//レンダーターゲット
	gpsDesc.NumRenderTargets	= 1;
	gpsDesc.RTVFormats[0]		= DXGI_FORMAT_R8G8B8A8_UNORM;

	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable		= true;
	gpsDesc.DepthStencilState.StencilEnable		= false;
	gpsDesc.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc			= D3D12_COMPARISON_FUNC_LESS;
	gpsDesc.DSVFormat							= DXGI_FORMAT_D32_FLOAT;

	//ラスタライザ
	gpsDesc.RasterizerState				= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;
	gpsDesc.RasterizerState.FillMode	= D3D12_FILL_MODE_SOLID;

	D3D12_RENDER_TARGET_BLEND_DESC renderBlDesc = {};
	renderBlDesc.BlendEnable			= true;
	renderBlDesc.BlendOp				= D3D12_BLEND_OP_ADD;
	renderBlDesc.BlendOpAlpha			= D3D12_BLEND_OP_ADD;
	renderBlDesc.SrcBlend				= D3D12_BLEND_SRC_ALPHA;
	renderBlDesc.DestBlend				= D3D12_BLEND_INV_SRC_ALPHA;
	renderBlDesc.SrcBlendAlpha			= D3D12_BLEND_ONE;
	renderBlDesc.DestBlendAlpha			= D3D12_BLEND_ZERO;
	renderBlDesc.RenderTargetWriteMask	= D3D12_COLOR_WRITE_ENABLE_ALL;

	//αブレンド
	D3D12_BLEND_DESC BlendDesc = {};
	BlendDesc.AlphaToCoverageEnable = false;
	BlendDesc.IndependentBlendEnable = false;
	BlendDesc.RenderTarget[0] = renderBlDesc;

	//その他
	gpsDesc.BlendState				= BlendDesc;
	gpsDesc.NodeMask				= 0;
	gpsDesc.SampleDesc.Count		= 1;
	gpsDesc.SampleDesc.Quality		= 0;
	//gpsDesc.SampleMask			= 0xffffffff;
	gpsDesc.SampleMask				= D3D12_COLOR_WRITE_ENABLE_ALL;
	gpsDesc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pipeline));

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

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_perapipeline));
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

void Wrapper::InitModelVertices()
{
	/*auto vdata = _model->GetverticesData();
	auto idata = _model->GetindexData();*/
	
	auto vdata = _pmxModel->GetverticesData();
	auto idata = _pmxModel->GetindexData();

	//verticesバッファ作成
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vdata.size() * sizeof(vdata[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_vertexModelBuffer)
	);

	//マップ
	D3D12_RANGE range = { 0,0 };
	//PMDvertex* _vBufferptr = nullptr;
	PMXVertex* _vBufferptr = nullptr;

	_vertexModelBuffer->Map(0, &range, (void**)&_vBufferptr);
	std::copy(vdata.begin(), vdata.end(), _vBufferptr);
	_vertexModelBuffer->Unmap(0, nullptr);

	_vbView.BufferLocation = _vertexModelBuffer->GetGPUVirtualAddress();
	_vbView.SizeInBytes = vdata.size() * sizeof(vdata[0]);
	_vbView.StrideInBytes = sizeof(vdata[0]);

	//インデックスバッファ作成
	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(idata.size() * sizeof(idata[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexModelBuffer)
	);

	//マップ
	unsigned short* Buffptr = nullptr;
	result = _indexModelBuffer->Map(0, &range, (void**)&Buffptr);

	std::copy(idata.begin(), idata.end(), Buffptr);
	_indexModelBuffer->Unmap(0, nullptr);

	_idxbView.BufferLocation = _indexModelBuffer->GetGPUVirtualAddress();
	_idxbView.SizeInBytes = idata.size() * sizeof(unsigned short);
	_idxbView.Format = DXGI_FORMAT_R16_UINT;
}

void Wrapper::InitDescriptorHeapDSV()
{
	auto &app = Application::GetInstance();

	//デスクリプターヒープ
	D3D12_DESCRIPTOR_HEAP_DESC _dsvDesc = {};
	_dsvDesc.Flags					= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	_dsvDesc.NodeMask				= 0;
	_dsvDesc.NumDescriptors			= 1;
	_dsvDesc.Type					= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	auto result = _dev->CreateDescriptorHeap(&_dsvDesc, IID_PPV_ARGS(&_dsvHeap));

	//深度バッファ
	D3D12_HEAP_PROPERTIES heappropDsv = {};
	heappropDsv.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heappropDsv.CreationNodeMask		= 0;
	heappropDsv.VisibleNodeMask			= 0;
	heappropDsv.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
	heappropDsv.Type					= D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC dsvDesc = {};
	dsvDesc.Alignment			= 0;
	dsvDesc.Width				= app.GetWIndowSize().w;
	dsvDesc.Height				= app.GetWIndowSize().h;
	dsvDesc.DepthOrArraySize	= 1;
	dsvDesc.MipLevels			= 0;
	dsvDesc.Format				= DXGI_FORMAT_R32_TYPELESS;
	dsvDesc.SampleDesc.Count	= 1;
	dsvDesc.SampleDesc.Quality	= 0;
	dsvDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsvDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	dsvDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	//クリアバリュー
	D3D12_CLEAR_VALUE clearValue;
	clearValue.DepthStencil.Depth	= 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	clearValue.Format				= DXGI_FORMAT_D32_FLOAT;

	result = _dev->CreateCommittedResource(
		&heappropDsv,
		D3D12_HEAP_FLAG_NONE,
		&dsvDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&_dsvBuff));

	//深度バッファービュー
	D3D12_DEPTH_STENCIL_VIEW_DESC _dsvVDesc = {};
	_dsvVDesc.Format						= DXGI_FORMAT_D32_FLOAT;
	_dsvVDesc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2D;
	_dsvVDesc.Texture2D.MipSlice			= 0;
	_dsvVDesc.Flags							= D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView(_dsvBuff, &_dsvVDesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Wrapper::InitPath1stRTVSRV()
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
	texDesc.Alignment				= 0;										//先頭からなので0
	texDesc.DepthOrArraySize		= 1;										//リソースが2Dで配列でもないので１
	texDesc.Dimension				= D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//何次元テクスチャか(TEXTURE2D)
	texDesc.Flags					= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;	//NONE
	texDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM;				//R8G8B8A8
	texDesc.Width					= app.GetWIndowSize().w;					//テクスチャ幅
	texDesc.Height					= app.GetWIndowSize().h;					//テクスチャ高さ
	texDesc.Layout					= D3D12_TEXTURE_LAYOUT_UNKNOWN;				//決定できないのでUNKNOWN
	texDesc.MipLevels				= 0;										//ミップ使ってないので0
	texDesc.SampleDesc.Count		= 1;
	texDesc.SampleDesc.Quality		= 0;

	float clearColor[] = { 0.5, 0.5, 0.5,1.f };

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth		= 1.0f; //最大値１
	clearValue.DepthStencil.Stencil		= 0;
	clearValue.Format					= DXGI_FORMAT_R8G8B8A8_UNORM;
	std::copy(std::begin(clearColor), std::end(clearColor), clearValue.Color);

	//リソース
	auto result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		&clearValue,
		IID_PPV_ARGS(&_1stPathBuff));

	//デスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HeapDesc.NodeMask					= 0;
	HeapDesc.NumDescriptors				= 1;								
	HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//ディスクリプタの型(レンダーターゲットビュー)

	//rtvデスクリプタヒープ作成
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_rtv1stDescHeap));

	//srvデスクリプタヒープ作成
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_srv1stDescHeap));

	//レンダーターゲットビュー作成
	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescrtvH = _rtv1stDescHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateRenderTargetView(_1stPathBuff, nullptr, HeapDescrtvH);

	//シェーダーリソースビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format							= DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels				= 1;
	srvDesc.Texture2D.MostDetailedMip		= 0;
	srvDesc.Texture2D.PlaneSlice			= 0;
	srvDesc.Texture2D.ResourceMinLODClamp	= 0.0F;
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescSrvH = _srv1stDescHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateShaderResourceView(_1stPathBuff, &srvDesc, HeapDescSrvH);
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
	_cmdList->IASetVertexBuffers(0, 1, &_vbView);
	_cmdList->IASetIndexBuffer(&_idxbView);

	//ボーンヒープ
	_cmdList->SetDescriptorHeaps(1, &_pmxModel->GetBoneHeap());
	_cmdList->SetGraphicsRootDescriptorTable(1, _pmxModel->GetBoneHeap()->GetGPUDescriptorHandleForHeapStart());

	//モデル表示
	unsigned int offset = 0;

	for (auto m : _pmxModel->GetmatData()) {
		_cmdList->DrawIndexedInstanced(m.face_vert_cnt, 1, offset, 0, 0);
		offset += m.face_vert_cnt;
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

	//モーション(アクション)
	//const char* mfilepath = ("Motion/pose.vmd");
	const char* mfilepath = ("Motion/swing2.vmd");
	//const char* mfilepath = ("Motion/charge.vmd.vmd");
	//const char* mfilepath = ("Motion/first.vmd");

	//モーション(ダンス)
	//const char* mfilepath = ("Motion/ELECTモーション_小さめ身長用.vmd");
	//const char* mfilepath = ("Motion/ストロボナイツモーション.vmd");
	//const char* mfilepath = ("Motion/えれくとりっくえんじぇぅ.vmd");
	//const char* mfilepath = ("Motion/ヤゴコロダンス.vmd");

	_model.reset(new PMDModel(cfilepath,_dev));

	_pmxModel.reset(new PMXModel(xfilepath, _dev));

	//_model->InitMotion(mfilepath,_dev);

	//_model->InitBone(_dev);

	_pmxModel->InitBone(_dev);

	_floor.reset(new Floor(_dev));

	_shadow.reset(new Shadow(_dev));

	InitModelVertices();
	
	//画像のやつ
	//InitVertices();

	InitVerticesPera();

	InitShader();

	InitTexture();

	InitRootSignature();

	InitPath1stRootSignature();

	_floor->InitRootSignature(_dev);

	_shadow->InitRootSignature(_dev);
	
	InitPipeline();

	_shadow->InitPipline(_dev);
}

Wrapper::~Wrapper()
{
}

void Wrapper::Update()
{
	unsigned char keyState[256];
	_camera->CameraUpdate(keyState);

	//_model->Update();
	_pmxModel->Update();

	auto heapStart = _rtv1stDescHeap->GetCPUDescriptorHandleForHeapStart();

	float clearColor[] = { 0.8f,0.8f,0.8f,1.0f };
	
	auto bbidx = _swapchain->GetCurrentBackBufferIndex();
	auto rtvHeapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//heapStart.ptr += (bbidx * rtvHeapSize);

	//コマンドのリセット
	auto result = _cmdAllocator->Reset();
	result = _cmdList->Reset(_cmdAllocator, nullptr);

	DrawLightView();

	//パイプラインのセット
	_cmdList->SetPipelineState(_pipeline);

	//ルートシグネチャのセット
	_cmdList->SetGraphicsRootSignature(_rootSignature);

	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	//バリアセット
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_1stPathBuff,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(1, &heapStart, false, &_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	//クリアレンダーターゲット
	_cmdList->ClearRenderTargetView(heapStart,clearColor,0, nullptr);

	//深度バッファをクリア
	_cmdList->ClearDepthStencilView(_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	
	//CBVデスクリプタヒープ設定
	_cmdList->SetDescriptorHeaps(1, &_camera->GetrgstDescHeap());
	_cmdList->SetGraphicsRootDescriptorTable(0, _camera->GetrgstDescHeap()->GetGPUDescriptorHandleForHeapStart());

	//ボーンヒープセット
	_cmdList->SetDescriptorHeaps(1, &_pmxModel->GetBoneHeap());
	_cmdList->SetGraphicsRootDescriptorTable(2, _pmxModel->GetBoneHeap()->GetGPUDescriptorHandleForHeapStart());

	//頂点セット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//モデル表示
	_cmdList->IASetVertexBuffers(0, 1, &_vbView);
	_cmdList->IASetIndexBuffer(&_idxbView);

	//_cmdList->SetDescriptorHeaps(1, &_model->GetMatHeap());
	_cmdList->SetDescriptorHeaps(1, &_pmxModel->GetMatHeap());

	unsigned int offset = 0;

	/*auto mathandle = _model->GetMatHeap()->GetGPUDescriptorHandleForHeapStart();*/
	auto mathandle = _pmxModel->GetMatHeap()->GetGPUDescriptorHandleForHeapStart();

	auto incriment_size = 
		_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	
	/*for (auto& m : _model->GetmatData()) {
		_cmdList->SetGraphicsRootDescriptorTable(1, mathandle);
		mathandle.ptr += incriment_size;
		_cmdList->DrawIndexedInstanced(m.face_vert_count, 1, offset, 0, 0);
		offset += m.face_vert_count;
	}*/
	
	for (auto& m : _pmxModel->GetmatData()) {
		_cmdList->SetGraphicsRootDescriptorTable(1, mathandle);
		mathandle.ptr += incriment_size;
		_cmdList->DrawIndexedInstanced(m.face_vert_cnt, 1, offset, 0, 0);
		offset += m.face_vert_cnt;
	}

	//床
	_cmdList->SetPipelineState(_floor->_GetPipeline());
	_cmdList->SetGraphicsRootSignature(_floor->GetRootSignature());
	
	_cmdList->SetPipelineState(_shadow->GetShadowPipeline());
	_cmdList->SetGraphicsRootSignature(_shadow->GetShadowRootSignature());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_floor->GetView());
	_cmdList->DrawInstanced(4, 1, 0, 0);;

	//バリア閉じ
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_1stPathBuff,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	_cmdList->Close();

	ExecuteCmd();

	//待ち
	WaitExcute();

	PeraUpdate();

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
			_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	//マルチパス用のルートシグネチャー
	_cmdList->SetGraphicsRootSignature(_perarootsigunature);

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
	_cmdList->SetDescriptorHeaps(1, &_srv1stDescHeap);
	_cmdList->SetGraphicsRootDescriptorTable(0, _srv1stDescHeap->GetGPUDescriptorHandleForHeapStart());

	_cmdList->SetDescriptorHeaps(1, &_shadow->GetSrvHeap());
	_cmdList->SetGraphicsRootDescriptorTable(0, _shadow->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//バッファービューのセット
	_cmdList->IASetVertexBuffers(0, 1, &_1stvbView);

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
