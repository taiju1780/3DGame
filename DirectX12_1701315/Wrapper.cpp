#include "Wrapper.h"
#include "Application.h"
#include "Camera.h"
#include "PMDModel.h"
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

	auto result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &vertexShader, nullptr);
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &pixelShader, nullptr);

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
	samplerDesc[0].AddressU				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressV				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressW				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
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

	samplerDesc[1] = samplerDesc[0];
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].ShaderRegister = 1;
	
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
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0 
		},

		{
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},	

		{
			"BONENO",
			0,
			DXGI_FORMAT_R16G16_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		{
			"WEIGHT",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT ,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0 
		}
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

	//その他
	gpsDesc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask				= 0;
	gpsDesc.SampleDesc.Count		= 1;
	gpsDesc.SampleDesc.Quality		= 0;
	//gpsDesc.SampleMask			= 0xffffffff;
	gpsDesc.SampleMask				= D3D12_COLOR_WRITE_ENABLE_ALL;
	gpsDesc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pipeline));
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

	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

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
	auto vdata = _model->GetverticesData();
	auto idata = _model->GetindexData();

	//verticesバッファ作成
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vdata.size() * VertexSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_vertexModelBuffer)
	);

	//マップ
	D3D12_RANGE range = { 0,0 };
	PMDvertex* _vBufferptr = nullptr;

	_vertexModelBuffer->Map(0, &range, (void**)&_vBufferptr);
	std::copy(vdata.begin(), vdata.end(), _vBufferptr);
	_vertexModelBuffer->Unmap(0, nullptr);

	_vbView.BufferLocation = _vertexModelBuffer->GetGPUVirtualAddress();
	_vbView.SizeInBytes = vdata.size() * VertexSize;
	_vbView.StrideInBytes = VertexSize;

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

	_camera.reset(new Camera(_dev));

	const char* cfilepath = ("Model/初音ミク.pmd");
	//const char* cfilepath = ("Model/巡音ルカ.pmd");
	//const char* cfilepath = ("Model/初音ミクmetal.pmd");
	//const char* cfilepath = ("Model/初音ミクXS改変雪桜-1.1/mikuXS桜ミク.pmd");
	//const char* cfilepath = ("Model/hibiki/我那覇響v1.pmd");
	//const char* cfilepath = ("Model/hibari/雲雀Ver1.10.pmd");
	//const char* cfilepath = ("Model/博麗霊夢/reimu_F01.pmd");

	//モーション(アクション)
	const char* mfilepath = ("Motion/pose.vmd");
	//const char* mfilepath = ("Motion/swing2.vmd");
	//const char* mfilepath = ("Motion/charge.vmd.vmd");
	//const char* mfilepath = ("Motion/first.vmd");

	//モーション(ダンス)
	//const char* mfilepath = ("Motion/ELECTモーション_小さめ身長用.vmd");
	//const char* mfilepath = ("Motion/ストロボナイツモーション.vmd");
	//const char* mfilepath = ("Motion/えれくとりっくえんじぇぅ.vmd");
	//const char* mfilepath = ("Motion/ヤゴコロダンス.vmd");

	_model.reset(new PMDModel(cfilepath,_dev));

	_model->InitMotion(mfilepath,_dev);

	_model->InitBone(_dev);

	InitModelVertices();
	
	//画像のやつ
	//InitVertices();

	InitShader();

	InitTexture();

	InitRootSignature();
	
	InitPipeline();
}

Wrapper::~Wrapper()
{
}

void Wrapper::Update()
{
	unsigned char keyState[256];
	_camera->CameraUpdate(keyState);

	_model->Update();

	auto heapStart = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	float clearColor[] = { 0.8f,0.8f,0.8f,1.0f };
	
	auto bbidx = _swapchain->GetCurrentBackBufferIndex();
	auto rtvHeapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	heapStart.ptr += (bbidx * rtvHeapSize);

	//コマンドのリセット
	auto result = _cmdAllocator->Reset();
	result = _cmdList->Reset(_cmdAllocator, nullptr);

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
		_backBuffers[bbidx],
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
	_cmdList->SetDescriptorHeaps(1, &_model->GetBoneHeap());
	_cmdList->SetGraphicsRootDescriptorTable(2, _model->GetBoneHeap()->GetGPUDescriptorHandleForHeapStart());

	//頂点セット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//モデル表示
	_cmdList->IASetVertexBuffers(0, 1, &_vbView);
	_cmdList->IASetIndexBuffer(&_idxbView);

	_cmdList->SetDescriptorHeaps(1, &_model->GetMatHeap());

	unsigned int offset = 0;

	auto mathandle = _model->GetMatHeap()->GetGPUDescriptorHandleForHeapStart();

	auto incriment_size = 
		_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	
	for (auto& m : _model->GetmatData()) {
		_cmdList->SetGraphicsRootDescriptorTable(1, mathandle);
		mathandle.ptr += incriment_size;
		_cmdList->DrawIndexedInstanced(m.face_vert_count, 1, offset, 0, 0);
		offset += m.face_vert_count;
	}

	//バリア閉じ
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbidx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	_cmdList->Close();

	ExecuteCmd();

	_swapchain->Present(0, 0);

	//待ち
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
