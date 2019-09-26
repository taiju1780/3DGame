#include "Wrapper.h"
#include "Application.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>

//リンク
#pragma comment(lib,"d3d12.lib") 
#pragma comment(lib,"dxgi.lib") 
#pragma comment (lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;

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
		{{-1.0f,1.0f,0.0f},{0,0}},
		{{1.0f,1.0f,0.0f},{1,0}},
		{{-1.0f,-1.0f,0.0f},{0,1}},
		{{1.0f,-1.0f,0.0f},{1,1}},
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
	_idxbView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_idxbView.Format = DXGI_FORMAT_R16_UINT;
	_idxbView.SizeInBytes = indices.size() * sizeof(indices[0]);
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
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.BorderColor				= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//エッジの色
	samplerDesc.Filter					= D3D12_FILTER_MIN_MAG_MIP_LINEAR;//特別なフィルタを使用しない
	samplerDesc.MaxLOD					= D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD					= 0.0f;
	samplerDesc.MipLODBias				= 0.0f;
	samplerDesc.ShaderRegister			= 0;
	samplerDesc.ShaderVisibility		= D3D12_SHADER_VISIBILITY_ALL;//どのくらいシェーダに見せるか
	samplerDesc.RegisterSpace			= 0;
	samplerDesc.MaxAnisotropy			= 0;
	samplerDesc.ComparisonFunc			= D3D12_COMPARISON_FUNC_NEVER;
	
	ID3DBlob* signature = nullptr;//ID3D12Blob=メモリオブジェクト
	ID3DBlob* error = nullptr;

	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};
	D3D12_ROOT_PARAMETER rootParam = {};

	//デスクリプタレンジの設定
	descTblRange[0].BaseShaderRegister					= 0;//レジスタ番号
	descTblRange[0].NumDescriptors						= 1;
	descTblRange[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblRange[0].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblRange[1].BaseShaderRegister					= 0;//レジスタ番号
	descTblRange[1].NumDescriptors						= 1;
	descTblRange[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[1].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParam.ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	rootParam.DescriptorTable.NumDescriptorRanges	= 2;
	rootParam.DescriptorTable.pDescriptorRanges		= descTblRange;
	
	//ルートシグネチャ
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags				= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pParameters			= &rootParam;
	rsd.pStaticSamplers		= &samplerDesc;
	rsd.NumParameters		= 1;
	rsd.NumStaticSamplers	= 1;

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
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
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
	gpsDesc.DepthStencilState.DepthEnable	= false;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DSVFormat;

	//ラスタライザ
	gpsDesc.RasterizerState				= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;
	gpsDesc.RasterizerState.FillMode	= D3D12_FILL_MODE_SOLID;

	//その他
	gpsDesc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask				= 0;
	gpsDesc.SampleDesc.Count		= 1;
	gpsDesc.SampleDesc.Quality		= 0;
	//gpsDesc.SampleMask				= 0xffffffff;
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
	texDesc.Width				= app.GetWIndowSize().w / 2;
	texDesc.Height				= app.GetWIndowSize().h / 2;
	texDesc.DepthOrArraySize	= 1;
	texDesc.MipLevels			= 0;
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
		&clearValue,
		IID_PPV_ARGS(&_texbuff));

	TexMetadata metadata;
	ScratchImage img;
	result = LoadFromWICFile(L"img/IMG_0164.png", WIC_FLAGS_NONE, &metadata, img);

	result = _texbuff->WriteToSubresource(0,nullptr,img.GetPixels(),metadata.width * 4,img.GetPixelsSize());


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

void Wrapper::InitConstants()
{
	auto wsize = Application::GetInstance().GetWIndowSize();

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = _dev->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_rgstDescHeap));

	//カメラ設定
	XMFLOAT3 eye(0, 15, -15);//視点
	XMFLOAT3 target(0, 10, 0);//注視点
	XMFLOAT3 up(0, 10, 10);//上ベクトル

	//ベクトル変換
	XMVECTOR veye = XMLoadFloat3(&eye);

	//左手系,ビュー行列
	_wvp._view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	//プロジェクション行列
	//画角、アスペクト比、ニア、ファー
	_wvp._projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,
		static_cast<float>(wsize.w) / static_cast<float>(wsize.h),
		0.1f,
		300);

	//ワールド行列
	angle = 0.0f;
	_wvp._world = XMMatrixRotationY(angle);

	size_t size = sizeof(_wvp);
	size = (size + 0xff)&~0xff;
	//size = size+(256 - size % 256)% 256
	//↑都合上256ビットごとに数えたい

	//コンスタントバッファ
	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_cBuff));

	//マップ
	XMMATRIX* mat = nullptr;
	result = _cBuff->Map(0, nullptr, (void**)&_mappedWvp);
	memcpy(_mappedWvp, &_wvp, sizeof(_wvp));
	_cBuff->Unmap(0, nullptr);

	//定数バッファビュー作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _cBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;

	_dev->CreateConstantBufferView(&cbvDesc, _rgstDescHeap->GetCPUDescriptorHandleForHeapStart());

	anglex = 0.0f;
	angley = 0.0f;
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

	InitConstants();

	InitVertices();

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
	auto heapStart = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	float clearColor[] = { 1.0f,0.0f,0.0f,1.0f };
	
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
	_cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);

	//クリア
	_cmdList->ClearRenderTargetView(heapStart,clearColor,0, nullptr);

	//テクスチャセット
	_cmdList->SetDescriptorHeaps(1, &_texsrvHeap);
	_cmdList->SetGraphicsRootDescriptorTable(0, _texsrvHeap->GetGPUDescriptorHandleForHeapStart());

	//頂点セット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_cmdList->IASetVertexBuffers(0, 1, &_vbView);

	_cmdList->IASetIndexBuffer(&_idxbView);
	_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	
	//バリア閉じ
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbidx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	_cmdList->Close();

	ExecuteCmd();

	//待ち
	WaitExcute();

	_swapchain->Present(1, 0);
}

void Wrapper::ExecuteCmd()
{
	ID3D12CommandList* _cmdLists[] = { _cmdList };
	_cmdQue->ExecuteCommandLists(1, _cmdLists);
	_cmdQue->Signal(_fence, ++_fenceValue);
}

void Wrapper::WaitExcute()
{
	while (_fence->GetCompletedValue() != _fenceValue);
}

void Wrapper::InitFence()
{
	_fenceValue = 0;
	auto result = _dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
}
