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

//�����N
#pragma comment(lib,"d3d12.lib") 
#pragma comment(lib,"dxgi.lib") 
#pragma comment (lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;

unsigned int VertexSize = 38;

struct Vertex {
	XMFLOAT3 pos; //���W
	XMFLOAT2 uv; //uv
};

void Wrapper::InitSwapChain()
{
	auto& app = Application::GetInstance();
	auto wsize = app.GetWIndowSize();

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width					= wsize.w;								//��ʕ�
	swapchainDesc.Height				= wsize.h;								//��ʍ���
	swapchainDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;			//�f�[�^�t�H�[�}�b�g
	swapchainDesc.Stereo				= false;								//�X�e���I�ɂ��邩�ǂ���
	swapchainDesc.SampleDesc.Count		= 1;									//�}���`�T���v�����O��
	swapchainDesc.SampleDesc.Quality	= 0;									//�C���[�W�͈̔̓��x��(0�`1)
	swapchainDesc.BufferCount			= 2;									//�o�b�t�@��
	swapchainDesc.BufferUsage			= DXGI_USAGE_BACK_BUFFER;				//�T�[�t�F�X�܂��̓��\�[�X���o�̓����_�[�^�[�Q�b�g�Ƃ��Ďg�p
	swapchainDesc.Scaling				= DXGI_SCALING_STRETCH;					//�T�C�Y���킹
	swapchainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;		//�\�����@
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
	//��������Queue�ƈꏏ
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
	descriptorHeapDesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //�����_�[�^�[�Q�b�g�r���[
	descriptorHeapDesc.NodeMask			= 0;
	descriptorHeapDesc.NumDescriptors	= 2; //�\��ʂƗ���ʕ�
	descriptorHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	auto result = _dev->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&_rtvDescHeap));

	//�擪�n���h�����擾
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescH = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();

	//�f�X�N���v�^�������̃T�C�Y���擾
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

	//�}���`�p�X�p�V�F�[�_
	auto result = D3DCompileFromFile(L"MultiPath.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &peravertexShader, nullptr);
	result = D3DCompileFromFile(L"MultiPath.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &perapixelShader, nullptr);
	
	//�}���`�p�X2�p�V�F�[�_
	result = D3DCompileFromFile(L"MultiPath2.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &pera2vertexShader, nullptr);
	result = D3DCompileFromFile(L"MultiPath2.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &pera2pixelShader, nullptr);

	//�r���[�|�[�g�ݒ�
	_viewport.TopLeftX = 0;
	_viewport.TopLeftY = 0;
	_viewport.Width = wsize.w;
	_viewport.Height = wsize.h;
	_viewport.MaxDepth = 1.0f;
	_viewport.MinDepth = 0.0f;

	//�V�U�[(�؂���)��`
	_scissorRect.left = 0;
	_scissorRect.top = 0;
	_scissorRect.right = wsize.w;
	_scissorRect.bottom = wsize.h;
}

void Wrapper::InitPipeline()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets	= 1;
	gpsDesc.RTVFormats[0]		= DXGI_FORMAT_R8G8B8A8_UNORM;

	//���X�^���C�U
	gpsDesc.RasterizerState				= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;
	gpsDesc.RasterizerState.FillMode	= D3D12_FILL_MODE_SOLID;

	//���̑�
	gpsDesc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask				= 0;
	gpsDesc.SampleDesc.Count		= 1;
	gpsDesc.SampleDesc.Quality		= 0;
	gpsDesc.SampleMask				= D3D12_COLOR_WRITE_ENABLE_ALL;
	gpsDesc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//1stPath
	//���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC Peralayouts[] = {
		{ "POSITION",
		0,
		DXGI_FORMAT_R32G32B32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0 },

		//uv���C�A�E�g
		{ "TEXCOORD",
		0,
		DXGI_FORMAT_R32G32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0 },
	};

	//���[�g�V�O�l�`���[�ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _perarootsigunature;
	gpsDesc.InputLayout.pInputElementDescs = Peralayouts;
	gpsDesc.InputLayout.NumElements = _countof(Peralayouts);

	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = false;

	//�V�F�[�_�n
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(peravertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(perapixelShader);

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_perapipeline));

	//2ndPath
	//���_���C�A�E�g
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
	};

	//���[�g�V�O�l�`���[�ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _pera2rootsigunature;
	gpsDesc.InputLayout.pInputElementDescs = Pera2layouts;
	gpsDesc.InputLayout.NumElements = _countof(Pera2layouts);

	//�V�F�[�_�n
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

	//�o���A�Z�b�g
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			_texbuff,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	//�摜�ǂݍ���
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

	//�o���A��
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_texbuff,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST));

	_cmdList->Close();

	ExecuteCmd();
	WaitExcute();

	//�f�X�N���v�^�[�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HeapDesc.NodeMask					= 0;
	HeapDesc.NumDescriptors				= 1;
	HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	//�����_�[�^�[�Q�b�g�f�X�N���v�^�[�q�[�v�쐬
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_texrtvHeap));

	//�V�F�[�_���\�[�X�r���[�f�X�N���v�^�[�q�[�v�쐬
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_texsrvHeap));

	//�����_�[�^�[�Q�b�g�r���[
	auto HeapDescrtvH = _texrtvHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateRenderTargetView(_texbuff, nullptr, HeapDescrtvH);

	//�V�F�[�_�[���\�[�X�r���[
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

	//�f�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HeapDesc.NodeMask					= 0;
	HeapDesc.NumDescriptors				= 2;
	HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//�f�B�X�N���v�^�̌^(�����_�[�^�[�Q�b�g�r���[)

	//rtv�f�X�N���v�^�q�[�v�쐬
	auto result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_rtv1stDescHeap));

	//srv�f�X�N���v�^�q�[�v�쐬
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_srv1stDescHeap));

	_1stPathBuffers.resize(2);

	//�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask		= 1;
	heapprop.VisibleNodeMask		= 1;
	heapprop.Type					= D3D12_HEAP_TYPE_CUSTOM;

	//�e�N�X�`���f�X�N
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Alignment			= 0;										//�擪����Ȃ̂�0
	texDesc.DepthOrArraySize	= 1;										//���\�[�X��2D�Ŕz��ł��Ȃ��̂łP
	texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//�������e�N�X�`����(TEXTURE2D)
	texDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;	//NONE
	texDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;				//R8G8B8A8
	texDesc.Width				= app.GetWIndowSize().w;					//�e�N�X�`����
	texDesc.Height				= app.GetWIndowSize().h;					//�e�N�X�`������
	texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;				//����ł��Ȃ��̂�UNKNOWN
	texDesc.MipLevels			= 0;										//�~�b�v�g���ĂȂ��̂�0
	texDesc.SampleDesc.Count	= 1;
	texDesc.SampleDesc.Quality	= 0;

	float clearColor[] = { 0.5, 0.5, 0.5,1.f };

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth	= 1.0f; //�ő�l�P
	clearValue.DepthStencil.Stencil = 0;
	clearValue.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	std::copy(std::begin(clearColor), std::end(clearColor), clearValue.Color);

	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescrtvH = _rtv1stDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescSrvH = _srv1stDescHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < _1stPathBuffers.size(); ++i) {
		
		//���\�[�X
		auto result = _dev->CreateCommittedResource(
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(&_1stPathBuffers[i]));

		//�����_�[�^�[�Q�b�g�r���[�쐬
		_dev->CreateRenderTargetView(_1stPathBuffers[i], nullptr, HeapDescrtvH);
		HeapDescrtvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		//�V�F�[�_�[���\�[�X�r���[�쐬
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
		XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1),//���� 
		XMFLOAT3(-1,1,0),XMFLOAT2(0,0),//���� 
		XMFLOAT3(1,-1,0),XMFLOAT2(1,1),//���� 
		XMFLOAT3(1,1,0),XMFLOAT2(1,0),//����
	};

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),	//CPU����GPU�֓]������p
		D3D12_HEAP_FLAG_NONE,								//���ʂȎw��Ȃ�
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),	//�T�C�Y
		D3D12_RESOURCE_STATE_GENERIC_READ,					//�悭�킩��Ȃ�
		nullptr,											//nullptr�ł���
		IID_PPV_ARGS(&_peraBuff));

	D3D12_RANGE range = { 0,0 };
	VertexTex* vBuffptr = nullptr;
	result = _peraBuff->Map(0, nullptr, (void**)&vBuffptr);
	memcpy(vBuffptr, vertices, sizeof(vertices));
	_peraBuff->Unmap(0, nullptr);

	_1stvbView.BufferLocation = _peraBuff->GetGPUVirtualAddress();
	_1stvbView.SizeInBytes = sizeof(vertices);						//�f�[�^�S�̂̃T�C�Y
	_1stvbView.StrideInBytes = sizeof(VertexTex);
}

void Wrapper::InitPath1stRootSignature()
{
	//�T���v����ݒ�
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//�G���J��Ԃ����(U����)
	samplerDesc.AddressV					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//�G���J��Ԃ����(V����)
	samplerDesc.AddressW					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//�G���J��Ԃ����(W����)
	samplerDesc.BorderColor					= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;				//�G�b�W�̐F(������)
	samplerDesc.Filter						= D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;					//���ʂȃt�B���^���g�p���Ȃ�
	samplerDesc.MaxLOD						= D3D12_FLOAT32_MAX;										//MIPMAP����Ȃ�
	samplerDesc.MinLOD						= 0.0;														//MIPMAP�����Ȃ�
	samplerDesc.MipLODBias					= 0.0f;														//MIPMAP�̃o�C�A�X
	samplerDesc.ShaderRegister				= 0;														//�g�p����V�F�[�_���W�X�^(�X���b�g)
	samplerDesc.ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;								//�ǂ̂��炢�̃f�[�^���V�F�[�_�Ɍ����邩(�S��)
	samplerDesc.RegisterSpace				= 0;														//0�ł���
	samplerDesc.MaxAnisotropy				= 0;														//Filter��Anisotropy�̎��̂ݗL��
	samplerDesc.ComparisonFunc				= D3D12_COMPARISON_FUNC_NEVER;								//���ɔ�r���Ȃ�(�ł͂Ȃ���ɔے�)

	//���[�g�V�O�l�`���[�̐���
	ID3DBlob* rootSignatureBlob = nullptr;	//���[�g�V�O�l�`�������邽�߂̍ޗ� 
	ID3DBlob* error = nullptr;	//�G���[�o�����̑Ώ�

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

	//�f�X�N���v�^�[�e�[�u���ݒ�
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

	//�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask		= 1;
	heapprop.VisibleNodeMask		= 1;
	heapprop.Type					= D3D12_HEAP_TYPE_CUSTOM;

	//�e�N�X�`���f�X�N
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Alignment			= 0;										//�擪����Ȃ̂�0
	texDesc.DepthOrArraySize	= 1;										//���\�[�X��2D�Ŕz��ł��Ȃ��̂łP
	texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//�������e�N�X�`����(TEXTURE2D)
	texDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;	//NONE
	texDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;				//R8G8B8A8
	texDesc.Width				= app.GetWIndowSize().w;					//�e�N�X�`����
	texDesc.Height				= app.GetWIndowSize().h;					//�e�N�X�`������
	texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;				//����ł��Ȃ��̂�UNKNOWN
	texDesc.MipLevels			= 0;										//�~�b�v�g���ĂȂ��̂�0
	texDesc.SampleDesc.Count	= 1;
	texDesc.SampleDesc.Quality	= 0;

	float clearColor[] = { 0.5, 0.5, 0.5,1.f };

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth	= 1.0f; //�ő�l�P
	clearValue.DepthStencil.Stencil = 0;
	clearValue.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	std::copy(std::begin(clearColor), std::end(clearColor), clearValue.Color);

	//���\�[�X
	auto result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(&_2ndPathBuff));

	//�f�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HeapDesc.NodeMask		= 0;
	HeapDesc.NumDescriptors = 1;
	HeapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//�f�B�X�N���v�^�̌^(�����_�[�^�[�Q�b�g�r���[)

	//rtv�f�X�N���v�^�q�[�v�쐬
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_rtv2ndDescHeap));

	//srv�f�X�N���v�^�q�[�v�쐬
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&_srv2ndDescHeap));

	//�����_�[�^�[�Q�b�g�r���[�쐬
	D3D12_CPU_DESCRIPTOR_HANDLE HeapDescrtvH = _rtv2ndDescHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateRenderTargetView(_2ndPathBuff, nullptr, HeapDescrtvH);

	//�V�F�[�_�[���\�[�X�r���[�쐬
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
		XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1),//���� 
		XMFLOAT3(-1,1,0),XMFLOAT2(0,0),//���� 
		XMFLOAT3(1,-1,0),XMFLOAT2(1,1),//���� 
		XMFLOAT3(1,1,0),XMFLOAT2(1,0),//����
	};

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),	//CPU����GPU�֓]������p
		D3D12_HEAP_FLAG_NONE,								//���ʂȎw��Ȃ�
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),	//�T�C�Y
		D3D12_RESOURCE_STATE_GENERIC_READ,					//�悭�킩��Ȃ�
		nullptr,											//nullptr�ł���
		IID_PPV_ARGS(&_pera2Buff));

	D3D12_RANGE range = { 0,0 };
	VertexTex* vBuffptr = nullptr;
	result = _pera2Buff->Map(0, nullptr, (void**)&vBuffptr);
	memcpy(vBuffptr, vertices, sizeof(vertices));
	_pera2Buff->Unmap(0, nullptr);

	_2ndvbView.BufferLocation = _pera2Buff->GetGPUVirtualAddress();
	_2ndvbView.SizeInBytes = sizeof(vertices);						//�f�[�^�S�̂̃T�C�Y
	_2ndvbView.StrideInBytes = sizeof(VertexTex);
}

void Wrapper::InitPath2ndRootSignature()
{
	//�T���v����ݒ�
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//�G���J��Ԃ����(U����)
	samplerDesc.AddressV				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//�G���J��Ԃ����(V����)
	samplerDesc.AddressW				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;							//�G���J��Ԃ����(W����)
	samplerDesc.BorderColor				= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;				//�G�b�W�̐F(������)
	samplerDesc.Filter					= D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;					//���ʂȃt�B���^���g�p���Ȃ�
	samplerDesc.MaxLOD					= D3D12_FLOAT32_MAX;										//MIPMAP����Ȃ�
	samplerDesc.MinLOD					= 0.0;														//MIPMAP�����Ȃ�
	samplerDesc.MipLODBias				= 0.0f;														//MIPMAP�̃o�C�A�X
	samplerDesc.ShaderRegister			= 0;														//�g�p����V�F�[�_���W�X�^(�X���b�g)
	samplerDesc.ShaderVisibility		= D3D12_SHADER_VISIBILITY_ALL;								//�ǂ̂��炢�̃f�[�^���V�F�[�_�Ɍ����邩(�S��)
	samplerDesc.RegisterSpace			= 0;														//0�ł���
	samplerDesc.MaxAnisotropy			= 0;														//Filter��Anisotropy�̎��̂ݗL��
	samplerDesc.ComparisonFunc			= D3D12_COMPARISON_FUNC_NEVER;								//���ɔ�r���Ȃ�(�ł͂Ȃ���ɔے�)

	//���[�g�V�O�l�`���[�̐���
	ID3DBlob* rootSignatureBlob = nullptr;	//���[�g�V�O�l�`�������邽�߂̍ޗ� 
	ID3DBlob* error = nullptr;	//�G���[�o�����̑Ώ�

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

	//�f�X�N���v�^�[�e�[�u���ݒ�
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

	//�f�X�N���v�^�[�q�[�v
	D3D12_DESCRIPTOR_HEAP_DESC _dsvDesc = {};
	_dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	_dsvDesc.NodeMask = 0;
	_dsvDesc.NumDescriptors = 1;
	_dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	auto result = _dev->CreateDescriptorHeap(&_dsvDesc, IID_PPV_ARGS(&_dsvHeap));

	//�[�x�o�b�t�@
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

	//�N���A�o�����[
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

	//�[�x�o�b�t�@�[�r���[
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

	//�r���[�|�[�g�A�V�U�[
	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	//�o���A�[
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_shadow->Getbuff(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE));

	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(0, nullptr, false, &_shadow->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart());

	//�[�x�o�b�t�@�̃N���A
	auto _sdsv = _shadow->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
	_cmdList->ClearDepthStencilView(_sdsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	//�q�[�v�Z�b�g
	_cmdList->SetDescriptorHeaps(1, &_camera->GetrgstDescHeap());
	_cmdList->SetGraphicsRootDescriptorTable(0, _camera->GetrgstDescHeap()->GetGPUDescriptorHandleForHeapStart());

	//�g�|���W�Z�b�g
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//�o�b�t�@�[�r���[�̃Z�b�g
	for (auto &pmxmodel : _pmxModels) {
		_cmdList->IASetVertexBuffers(0, 1, &pmxmodel->GetvView());
		_cmdList->IASetIndexBuffer(&pmxmodel->GetidxbView());

		//�{�[���q�[�v
		_cmdList->SetDescriptorHeaps(1, &pmxmodel->GetBoneHeap());
		_cmdList->SetGraphicsRootDescriptorTable(1, pmxmodel->GetBoneHeap()->GetGPUDescriptorHandleForHeapStart());

		//���f���\��
		unsigned int offset = 0;

		for (auto m : pmxmodel->GetMatData()) {
			_cmdList->DrawIndexedInstanced(m.face_vert_cnt, 1, offset, 0, 0);
			offset += m.face_vert_cnt;
		}
	}

	//�o���A�[
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
	//�t�B�[�`���[���x��
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
		//���̒����� NVIDIA �̓z��T��
		for (auto adpt : adapters) {
			DXGI_ADAPTER_DESC adesc = {};
			adpt->GetDesc(&adesc);
			std::wstring strDesc = adesc.Description;
			if (strDesc.find(L"NVIDIA") != std::string::npos) {//NVIDIA�A�_�v�^������ 
				adapter = adpt;
				break;
			}
		}
	}

	//�f�o�C�X�̏�����
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

	//const char* cfilepath = ("Model/�����~�N.pmd");
	//const char* cfilepath = ("Model/�������J.pmd");
	//const char* cfilepath = ("Model/�����~�Nmetal.pmd");
	const char* cfilepath = ("Model/�����~�NXS���ϐ��-1.1/mikuXS���~�N.pmd");
	//const char* cfilepath = ("Model/hibiki/��ߔe��v1.pmd");
	//const char* cfilepath = ("Model/hibari/�_��Ver1.10.pmd");
	//const char* cfilepath = ("Model/����얲/reimu_F01.pmd");

	//PMXModel
	/////////////////////////////////////////////////////////
	//const char* xfilepath = ("PMXModel/m_GUMI_V3_201306/GUMI��_V3.pmx");
	//const char* xfilepath = ("PMXModel/���у��[�~�A/���у��[�~�A.pmx");
	//const char* xfilepath = ("PMXModel/YYB�����Ϗ����~�N(PRT.B-Cos)/YYB�����Ϗ����~�N(PRT.B-Cos).pmx");
	//const char* xfilepath = ("PMXModel/�����ƃ��� Ver. 1.02/Rem Ver. 1.02.pmx");
	//const char* xfilepath = ("PMXModel/�����ƃ��� Ver. 1.02/Ram.pmx");
	const char* xfilepath = ("PMXModel/na_2b_0407h/na_2b_0407.pmx");
	//const char* x2filepath = ("PMXModel/���у��[�~�A/���у��[�~�A.pmx");
	const char* x2filepath = ("PMXModel/TB/TB.pmx");

	//���[�V����(�A�N�V����)
	//const char* mfilepath = ("Motion/pose.vmd");
	//const char* mfilepath = ("Motion/swing2.vmd");
	//const char* mfilepath = ("Motion/charge.vmd.vmd");
	//const char* mfilepath = ("Motion/first.vmd");

	//���[�V����(�_���X)
	//const char* mfilepath = ("Motion/45�bGUMI.vmd");
	const char* mfilepath = ("Motion/�X�g���{�i�C�c���[�V����.vmd");
	//const char* m2filepath = ("Motion/���ꂭ�Ƃ�������񂶂���.vmd");
	//const char* mfilepath = ("Motion/���S�R���_���X.vmd");
	const char* m2filepath = ("Motion/45�bMIKU.vmd");

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

	//�R�}���h�̃��Z�b�g
	auto result = _cmdAllocator->Reset();
	result = _cmdList->Reset(_cmdAllocator, nullptr);

	DrawLightView();

	for (int i = 0; i < _1stPathBuffers.size(); ++i) {
		//�o���A�Z�b�g
		_cmdList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
			_1stPathBuffers[i],
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(_1stPathBuffers.size(), &heapStart, true, &_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	

	for (int i = 0; i < _1stPathBuffers.size(); ++i) {
		//�N���A�����_�[�^�[�Q�b�g
		_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);
		heapStart.ptr += rtvHeapSize;
	}

	//�[�x�o�b�t�@���N���A
	_cmdList->ClearDepthStencilView(_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	for (auto &xmodel : _pmxModels) {
		xmodel->Draw(_dev, _cmdList, _camera, _rtv1stDescHeap);
	}

	//��
	_cmdList->SetPipelineState(_floor->_GetPipeline());
	_cmdList->SetGraphicsRootSignature(_floor->GetRootSignature());

	_cmdList->SetDescriptorHeaps(1, &_camera->GetrgstDescHeap());
	_cmdList->SetGraphicsRootDescriptorTable(0, _camera->GetrgstDescHeap()->GetGPUDescriptorHandleForHeapStart());

	//shadow�p
	_cmdList->SetDescriptorHeaps(1, &_shadow->GetSrvHeap());
	_cmdList->SetGraphicsRootDescriptorTable(1, _shadow->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_floor->GetView());
	_cmdList->DrawInstanced(4, 1, 0, 0);

	for (int i = 0; i < _1stPathBuffers.size(); ++i) {
		//�o���A��
		_cmdList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
			_1stPathBuffers[i],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	_cmdList->Close();

	ExecuteCmd();

	//�҂�
	WaitExcute();

	PeraUpdate();

	Pera2Update();

	_swapchain->Present(0, 0);
}

void Wrapper::PeraUpdate()
{
	auto result = _cmdAllocator->Reset();							//�A���P�[�^�[�̃��Z�b�g
	result = _cmdList->Reset(_cmdAllocator, _perapipeline);		//�R�}���h���X�g�̃��Z�b�g

	//���݂̃o�b�N�o�b�t�@�̃C���f�b�N�X
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			_2ndPathBuff,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	//�}���`�p�X�p�̃��[�g�V�O�l�`���[
	_cmdList->SetGraphicsRootSignature(_perarootsigunature);

	//�r���[�|�[�g�A�V�U�[
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

	//�����_�[�^�[�Q�b�g�w��
	auto rtv = _rtv2ndDescHeap->GetCPUDescriptorHandleForHeapStart();

	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(1, &rtv, false, nullptr);

	//�N���A�J���[�ݒ�
	float clearColor[] = { 0.5, 0.5, 0.5,1.f };

	//�����_�[�^�[�Q�b�g�̃N���A
	_cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

	//�y���|��
	_cmdList->SetDescriptorHeaps(1, &_srv1stDescHeap);
	_cmdList->SetGraphicsRootDescriptorTable(0, _srv1stDescHeap->GetGPUDescriptorHandleForHeapStart());

	_cmdList->SetDescriptorHeaps(1, &_shadow->GetSrvHeap());
	_cmdList->SetGraphicsRootDescriptorTable(1, _shadow->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//�o�b�t�@�[�r���[�̃Z�b�g
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

	//�o���A�[
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			_2ndPathBuff,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));

	_cmdList->Close();		//�R�}���h�̃N���[�Y

	ExecuteCmd();
	WaitExcute();
}

void Wrapper::Pera2Update()
{
	auto result = _cmdAllocator->Reset();							//�A���P�[�^�[�̃��Z�b�g
	result = _cmdList->Reset(_cmdAllocator, _pera2pipeline);		//�R�}���h���X�g�̃��Z�b�g

	//���݂̃o�b�N�o�b�t�@�̃C���f�b�N�X
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	//�}���`�p�X�p�̃��[�g�V�O�l�`���[
	_cmdList->SetGraphicsRootSignature(_pera2rootsigunature);

	//�r���[�|�[�g�A�V�U�[
	_viewport.Width = _2ndPathBuff->GetDesc().Width;
	_viewport.Height = _2ndPathBuff->GetDesc().Height;

	_scissorRect.right = _2ndPathBuff->GetDesc().Width;
	_scissorRect.bottom = _2ndPathBuff->GetDesc().Height;

	//�r���[�|�[�g�A�V�U�[
	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	//�����_�[�^�[�Q�b�g�w��
	auto rtv = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(1, &rtv, false, nullptr);

	//�N���A�J���[�ݒ�
	float clearColor[] = { 0.5, 0.5, 0.5,1.f };

	//�����_�[�^�[�Q�b�g�̃N���A
	_cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

	//�y���|��
	_cmdList->SetDescriptorHeaps(1, &_srv2ndDescHeap);
	_cmdList->SetGraphicsRootDescriptorTable(0, _srv2ndDescHeap->GetGPUDescriptorHandleForHeapStart());

	_cmdList->SetDescriptorHeaps(1, &_shadow->GetSrvHeap());
	_cmdList->SetGraphicsRootDescriptorTable(1, _shadow->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//�o�b�t�@�[�r���[�̃Z�b�g
	_cmdList->IASetVertexBuffers(0, 1, &_2ndvbView);

	_cmdList->DrawInstanced(4, 1, 0, 0);

	//�o���A�[
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	_cmdList->Close();		//�R�}���h�̃N���[�Y

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
