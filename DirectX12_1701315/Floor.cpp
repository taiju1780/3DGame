#include "Floor.h"
#include <DirectXMath.h>
#include "Application.h"
#include <d3dcompiler.h>

#pragma comment (lib,"d3dcompiler.lib")

using namespace DirectX;

struct VertexFloor {
	XMFLOAT3 pos;//���_���W
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
		XMFLOAT3(-edge,0,edge),XMFLOAT2(0,0),XMFLOAT3(0,1,0),		//���� 
		XMFLOAT3(-edge, 0,-edge),XMFLOAT2(0, 1),XMFLOAT3(0,1,0),	//���� 
		XMFLOAT3(edge,0,edge),XMFLOAT2(1,0),XMFLOAT3(0,1,0),		//����
		XMFLOAT3(edge,0,-edge),XMFLOAT2(1,1),XMFLOAT3(0,1,0),		//���� 
	};

	//��
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),	//CPU����GPU�֓]������p
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
	_fvbView.SizeInBytes = sizeof(floorvertices);					//�f�[�^�S�̂̃T�C�Y
	_fvbView.StrideInBytes = sizeof(VertexFloor);					//���̃A�h���X�܂ł̋���//���̃A�h���X�܂ł̋���
}

void Floor::InitShaders()
{
	HRESULT result = S_OK;

	//��
	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &floorVertexShader, nullptr);

	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &floorPixelShader, nullptr);
}

void Floor::InitRootSignature(ID3D12Device* _dev)
{
	//�T���v����ݒ�
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;							//�G���J��Ԃ����(U����)
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;							//�G���J��Ԃ����(V����)
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;							//�G���J��Ԃ����(W����)
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;				//�G�b�W�̐F(������)
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;								//���ʂȃt�B���^���g�p���Ȃ�
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;												//MIPMAP����Ȃ�
	samplerDesc.MinLOD = 0.0;															//MIPMAP�����Ȃ�
	samplerDesc.MipLODBias = 0.0f;														//MIPMAP�̃o�C�A�X
	samplerDesc.ShaderRegister = 0;														//�g�p����V�F�[�_���W�X�^(�X���b�g)
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;							//�ǂ̂��炢�̃f�[�^���V�F�[�_�Ɍ����邩(�S��)
	samplerDesc.RegisterSpace = 0;														//0�ł���
	samplerDesc.MaxAnisotropy = 0;														//Filter��Anisotropy�̎��̂ݗL��
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;							//���ɔ�r���Ȃ�(�ł͂Ȃ���ɔے�)

	//���[�g�V�O�l�`���[�̐���
	ID3DBlob* rootSignatureBlob = nullptr;	//���[�g�V�O�l�`�������邽�߂̍ޗ� 
	ID3DBlob* error = nullptr;	//�G���[�o�����̑Ώ�

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

	//�f�X�N���v�^�[�e�[�u���ݒ�
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
	//���_���C�A�E�g
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

		//uv���C�A�E�g
		{ 
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//�@��
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

	//���[�g�V�O�l�`���[�ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _floorRootSignature;
	gpsDesc.InputLayout.pInputElementDescs = FloorLayouts;
	gpsDesc.InputLayout.NumElements = _countof(FloorLayouts);

	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//�V�F�[�_�n
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(floorVertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(floorPixelShader);

	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets = 1;	//�^�[�Q�b�g���Ɛݒ肷��t�H�[�}�b�g���͈�v������
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//���X�^���C�U
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;		//����
	gpsDesc.SampleDesc.Quality = 0;		//����
	gpsDesc.SampleMask = 0xffffffff;	//�S���P

	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_floorPipeline));
}

