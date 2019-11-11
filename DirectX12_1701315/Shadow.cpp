#include "Shadow.h"
#include "Application.h"
#include <d3dcompiler.h>

#pragma comment (lib,"d3dcompiler.lib")

void Shadow::InitShaders()
{
	auto result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &_shadowVshader, nullptr);

	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &_shadowPshader, nullptr);
}

size_t Shadow::RoundupPowerOf2(size_t size)
{
	size_t bit = 0x8000000;
	for (size_t i = 31; i >= 0; --i) {		//一個ずつビットを下ろしていって
		if (size& bit)break;				//立っていたらそこで抜ける
		bit >>= 1;
	}
	return bit + (bit%size);
}

void Shadow::InitRootSignature(ID3D12Device *_dev)
{
	//サンプラを設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;					//絵が繰り返される(U方向)
	samplerDesc.AddressV					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;					//絵が繰り返される(V方向)
	samplerDesc.AddressW					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;					//絵が繰り返される(W方向)
	samplerDesc.BorderColor					= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;		//エッジの色(黒透明)
	samplerDesc.Filter						= D3D12_FILTER_MIN_MAG_MIP_LINEAR;	//特別なフィルタを使用しない
	samplerDesc.MaxLOD						= D3D12_FLOAT32_MAX;				//MIPMAP上限なし
	samplerDesc.MinLOD						= 0.0;								//MIPMAP下限なし
	samplerDesc.MipLODBias					= 0.0f;								//MIPMAPのバイアス
	samplerDesc.ShaderRegister				= 0;								//使用するシェーダレジスタ(スロット)
	samplerDesc.ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;		//どのくらいのデータをシェーダに見せるか(全部)
	samplerDesc.RegisterSpace				= 0;								//0でおｋ
	samplerDesc.MaxAnisotropy				= 0;								//FilterがAnisotropyの時のみ有効
	samplerDesc.ComparisonFunc				= D3D12_COMPARISON_FUNC_NEVER;		//特に比較しない(ではなく常に否定)

																	//ルートシグネチャーの生成
	ID3DBlob* rootSignatureBlob = nullptr;	//ルートシグネチャをつくるための材料 
	ID3DBlob* error = nullptr;	//エラー出た時の対処

	D3D12_DESCRIPTOR_RANGE descTblrange[2] = {};
	D3D12_ROOT_PARAMETER rootparam[2] = {};

	//b0
	descTblrange[0].NumDescriptors						= 1;
	descTblrange[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblrange[0].BaseShaderRegister					= 0;
	descTblrange[0].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//b1
	descTblrange[1].NumDescriptors						= 1;
	descTblrange[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblrange[1].BaseShaderRegister					= 1;
	descTblrange[1].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//デスクリプターテーブル設定
	rootparam[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	rootparam[0].DescriptorTable.NumDescriptorRanges	= 1;
	rootparam[0].DescriptorTable.pDescriptorRanges		= descTblrange;

	//ボーン用
	rootparam[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_VERTEX;
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

	result = _dev->CreateRootSignature(0,
		rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&_shadowRootSignature));
}

void Shadow::InitPipline(ID3D12Device *_dev)
{
	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC ShadowLayouts[] = {
		{ "POSITION",
		0,
		DXGI_FORMAT_R32G32B32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0 },

		//法線
		{ "NORMAL",
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

		//ボーン
		{ "BONENO",
		0,
		DXGI_FORMAT_R16G16_UINT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0 },

		//重み
		{ "WEIGHT",
		0,
		DXGI_FORMAT_R8_UINT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT ,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//ルートシグネチャーと頂点レイアウト
	gpsDesc.pRootSignature					= _shadowRootSignature;
	gpsDesc.InputLayout.pInputElementDescs	= ShadowLayouts;
	gpsDesc.InputLayout.NumElements			= _countof(ShadowLayouts);

	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable		= true;
	gpsDesc.DepthStencilState.StencilEnable		= false;
	gpsDesc.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc			= D3D12_COMPARISON_FUNC_LESS;
	gpsDesc.DSVFormat							= DXGI_FORMAT_D32_FLOAT;

	//シェーダ系
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_shadowVshader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_shadowPshader);

	//レンダーターゲット
	gpsDesc.NumRenderTargets = 1;	//ターゲット数と設定するフォーマット数は一致させる
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//その他
	gpsDesc.BlendState			= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask			= 0;
	gpsDesc.SampleDesc.Count	= 1;			//いる
	gpsDesc.SampleDesc.Quality	= 0;			//いる
	gpsDesc.SampleMask			= 0xffffffff;		//全部１
	//gpsDesc.Flags;						//デフォルトでおけ

	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_shadowPipeline));
}

void Shadow::CreateDSVSRV(ID3D12Device *_dev)
{
	auto& app = Application::GetInstance();
	auto size = RoundupPowerOf2(max(app.GetWIndowSize().w, app.GetWIndowSize().h));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.NumDescriptors		= 1;
	heapDesc.NodeMask			= 0;

	auto result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_shadowDsvHeap));

	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_shadowSrvHeap));

	//深度バッファ
	D3D12_HEAP_PROPERTIES heappropDsv = {};
	heappropDsv.Type					= D3D12_HEAP_TYPE_DEFAULT;
	heappropDsv.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heappropDsv.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
	heappropDsv.CreationNodeMask		= 0;
	heappropDsv.VisibleNodeMask			= 0;

	D3D12_RESOURCE_DESC dsvDesc = {};
	dsvDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsvDesc.Width				= size;
	dsvDesc.Height				= size;
	dsvDesc.DepthOrArraySize	= 1;
	dsvDesc.MipLevels			= 1;
	dsvDesc.Format				= DXGI_FORMAT_R32_TYPELESS;
	dsvDesc.SampleDesc.Count	= 1;
	dsvDesc.SampleDesc.Quality	= 0;
	dsvDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsvDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE _depthClearValue = {};
	_depthClearValue.DepthStencil.Depth		= 1.0f; //最大値１
	_depthClearValue.DepthStencil.Stencil	= 0;
	_depthClearValue.Format					= DXGI_FORMAT_D32_FLOAT;

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&dsvDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&_depthClearValue,
		IID_PPV_ARGS(&_shadowbuff));

	//深度バッファービュー
	D3D12_DEPTH_STENCIL_VIEW_DESC _dsvDesc = {};
	_dsvDesc.Format				= DXGI_FORMAT_D32_FLOAT;
	_dsvDesc.ViewDimension		= D3D12_DSV_DIMENSION_TEXTURE2D;
	_dsvDesc.Texture2D.MipSlice = 0;
	_dsvDesc.Flags				= D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescDsvH = _shadowDsvHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateDepthStencilView(_shadowbuff, &_dsvDesc, HeapDescDsvH);

	//シェーダーリソースビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format							= DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels				= 1;
	srvDesc.Texture2D.MostDetailedMip		= 0;
	srvDesc.Texture2D.PlaneSlice			= 0;
	srvDesc.Texture2D.ResourceMinLODClamp	= 0.0f;
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescSrvH = _shadowSrvHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateShaderResourceView(_shadowbuff, &srvDesc, HeapDescSrvH);
}

Shadow::Shadow(ID3D12Device *_dev)
{
	InitShaders();
	CreateDSVSRV(_dev);
}

Shadow::~Shadow()
{
}

ID3D12PipelineState *& Shadow::GetShadowPipeline()
{
	return _shadowPipeline;
}

ID3D12RootSignature *& Shadow::GetShadowRootSignature()
{
	return _shadowRootSignature;
}

ID3D12DescriptorHeap *& Shadow::GetDsvHeap()
{
	return _shadowDsvHeap;
}

ID3D12DescriptorHeap *& Shadow::GetSrvHeap()
{
	return _shadowSrvHeap;
}

ID3D12Resource *& Shadow::Getbuff()
{
	return _shadowbuff;
}
