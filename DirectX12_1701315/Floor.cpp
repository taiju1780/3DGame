#include "Floor.h"
#include <DirectXMath.h>
#include "Application.h"
#include <d3dcompiler.h>

#pragma comment (lib,"d3dcompiler.lib")

using namespace DirectX;

struct VertexFloor {
	XMFLOAT3 pos;//頂点座標
	XMFLOAT2 uv;
	XMFLOAT3 normal;
};

Floor::Floor(ID3D12Device* _dev)
{
	InitShaders();
	InitVerticesFloor(_dev);
}


Floor::~Floor()
{
}

void Floor::InitVerticesFloor(ID3D12Device* _dev)
{
	auto edge = 100;
	VertexFloor floorvertices[] = {
		XMFLOAT3(-edge,0,edge),XMFLOAT2(0,0),XMFLOAT3(0,1,0),		//床面 
		XMFLOAT3(-edge, 0,-edge),XMFLOAT2(0, 1),XMFLOAT3(0,1,0),	//床面 
		XMFLOAT3(edge,0,edge),XMFLOAT2(1,0),XMFLOAT3(0,1,0),		//床面
		XMFLOAT3(edge,0,-edge),XMFLOAT2(1,1),XMFLOAT3(0,1,0),		//床面 
	};

	//床
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),	//CPUからGPUへ転送する用
		D3D12_HEAP_FLAG_NONE,								
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(floorvertices)),	
		D3D12_RESOURCE_STATE_GENERIC_READ,					
		nullptr,											
		IID_PPV_ARGS(&_floorbuff));

	VertexFloor* fBuffptr = nullptr;
	result = _floorbuff->Map(0, nullptr, (void**)&fBuffptr);
	memcpy(fBuffptr, floorvertices, sizeof(floorvertices));
	_floorbuff->Unmap(0, nullptr);

	_fvbView.BufferLocation = _floorbuff->GetGPUVirtualAddress();
	_fvbView.SizeInBytes = sizeof(floorvertices);					//データ全体のサイズ
	_fvbView.StrideInBytes = sizeof(VertexFloor);					//次のアドレスまでの距離//次のアドレスまでの距離
}

void Floor::InitShaders()
{
	HRESULT result = S_OK;

	//床
	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &floorVertexShader, nullptr);

	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &floorPixelShader, nullptr);
}

void Floor::InitRootSignature(ID3D12Device* _dev)
{
	//サンプラを設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;							//絵が繰り返される(U方向)
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;							//絵が繰り返される(V方向)
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;							//絵が繰り返される(W方向)
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;				//エッジの色(黒透明)
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;								//特別なフィルタを使用しない
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;												//MIPMAP上限なし
	samplerDesc.MinLOD = 0.0;															//MIPMAP下限なし
	samplerDesc.MipLODBias = 0.0f;														//MIPMAPのバイアス
	samplerDesc.ShaderRegister = 0;														//使用するシェーダレジスタ(スロット)
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;							//どのくらいのデータをシェーダに見せるか(全部)
	samplerDesc.RegisterSpace = 0;														//0でおｋ
	samplerDesc.MaxAnisotropy = 0;														//FilterがAnisotropyの時のみ有効
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;							//特に比較しない(ではなく常に否定)

	//ルートシグネチャーの生成
	ID3DBlob* rootSignatureBlob = nullptr;	//ルートシグネチャをつくるための材料 
	ID3DBlob* error = nullptr;	//エラー出た時の対処

	D3D12_DESCRIPTOR_RANGE descTblrange[2] = {};
	D3D12_ROOT_PARAMETER rootparam[2] = {};

	//"b0"
	descTblrange[0].NumDescriptors = 1;
	descTblrange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblrange[0].BaseShaderRegister = 0;
	descTblrange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//depth
	descTblrange[1].NumDescriptors = 1;
	descTblrange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblrange[1].BaseShaderRegister = 0;
	descTblrange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//デスクリプターテーブル設定
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblrange[0];

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootparam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblrange[1];

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pParameters = rootparam;
	rsd.NumParameters = 2;
	rsd.pStaticSamplers = &samplerDesc;
	rsd.NumStaticSamplers = 1;

	auto result = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&rootSignatureBlob,
		&error);

	result = _dev->CreateRootSignature(0,
		rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&_floorRootSignature));
}

ID3D12RootSignature *& Floor::GetRootSignature()
{
	return _floorRootSignature;
}

ID3D12PipelineState *& Floor::_GetPipeline()
{
	return _floorPipeline;
}

D3D12_VERTEX_BUFFER_VIEW & Floor::GetView()
{
	return _fvbView;
}

void Floor::InitPiplineState(ID3D12Device* _dev)
{
	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC FloorLayouts[] = {
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

		//法線
		{ 
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//ルートシグネチャーと頂点レイアウト
	gpsDesc.pRootSignature = _floorRootSignature;
	gpsDesc.InputLayout.pInputElementDescs = FloorLayouts;
	gpsDesc.InputLayout.NumElements = _countof(FloorLayouts);

	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//シェーダ系
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(floorVertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(floorPixelShader);

	//レンダーターゲット
	gpsDesc.NumRenderTargets = 1;	//ターゲット数と設定するフォーマット数は一致させる
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//その他
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;		//いる
	gpsDesc.SampleDesc.Quality = 0;		//いる
	gpsDesc.SampleMask = 0xffffffff;	//全部１

	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_floorPipeline));
}

