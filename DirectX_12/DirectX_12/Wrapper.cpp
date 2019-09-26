#include "Wrapper.h"
#include "Application.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>

//�����N
#pragma comment(lib,"d3d12.lib") 
#pragma comment(lib,"dxgi.lib") 
#pragma comment (lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;

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

void Wrapper::InitVertices()
{
	Vertex vertices[] = {
		{{-1.0f,1.0f,0.0f},{0,0}},
		{{1.0f,1.0f,0.0f},{1,0}},
		{{-1.0f,-1.0f,0.0f},{0,1}},
		{{1.0f,-1.0f,0.0f},{1,1}},
	};

	//���\�[�X�̏�����
	auto result = _dev->CreateCommittedResource(
	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_vertexBuffer));

	//�}�b�v���ă��������m��
	Vertex* vBufferptr = nullptr;
	result = _vertexBuffer->Map(0, nullptr, (void**)&vBufferptr);
	memcpy(vBufferptr, vertices, sizeof(vertices));
	_vertexBuffer->Unmap(0, nullptr);

	//�o�b�t�@�[�r���[������
	_vbView.StrideInBytes = sizeof(Vertex);
	_vbView.SizeInBytes = sizeof(vertices);
	_vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();

	//�����_�p�C���f�b�N�X
	std::vector<unsigned short> indices = { 0,2,1,2,3,1 };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer));

	//�}�b�v
	short* idxBufptr = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxBufptr);
	std::copy(indices.begin(), indices.end(), idxBufptr);
	_indexBuffer->Unmap(0, nullptr);

	//�o�b�t�@�r���[�̐ݒ�
	_idxbView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_idxbView.Format = DXGI_FORMAT_R16_UINT;
	_idxbView.SizeInBytes = indices.size() * sizeof(indices[0]);
}

void Wrapper::InitShader()
{
	auto wsize = Application::GetInstance().GetWIndowSize();

	auto result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &vertexShader, nullptr);
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &pixelShader, nullptr);

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

void Wrapper::InitRootSignature()
{
	//�T���v��
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW				= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.BorderColor				= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//�G�b�W�̐F
	samplerDesc.Filter					= D3D12_FILTER_MIN_MAG_MIP_LINEAR;//���ʂȃt�B���^���g�p���Ȃ�
	samplerDesc.MaxLOD					= D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD					= 0.0f;
	samplerDesc.MipLODBias				= 0.0f;
	samplerDesc.ShaderRegister			= 0;
	samplerDesc.ShaderVisibility		= D3D12_SHADER_VISIBILITY_ALL;//�ǂ̂��炢�V�F�[�_�Ɍ����邩
	samplerDesc.RegisterSpace			= 0;
	samplerDesc.MaxAnisotropy			= 0;
	samplerDesc.ComparisonFunc			= D3D12_COMPARISON_FUNC_NEVER;
	
	ID3DBlob* signature = nullptr;//ID3D12Blob=�������I�u�W�F�N�g
	ID3DBlob* error = nullptr;

	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};
	D3D12_ROOT_PARAMETER rootParam = {};

	//�f�X�N���v�^�����W�̐ݒ�
	descTblRange[0].BaseShaderRegister					= 0;//���W�X�^�ԍ�
	descTblRange[0].NumDescriptors						= 1;
	descTblRange[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblRange[0].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblRange[1].BaseShaderRegister					= 0;//���W�X�^�ԍ�
	descTblRange[1].NumDescriptors						= 1;
	descTblRange[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[1].OffsetInDescriptorsFromTableStart	= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParam.ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	rootParam.DescriptorTable.NumDescriptorRanges	= 2;
	rootParam.DescriptorTable.pDescriptorRanges		= descTblRange;
	
	//���[�g�V�O�l�`��
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

	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature					= _rootSignature;
	gpsDesc.InputLayout.pInputElementDescs	= layout;
	gpsDesc.InputLayout.NumElements			= _countof(layout);

	//�V�F�[�_
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);

	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets	= 1;
	gpsDesc.RTVFormats[0]		= DXGI_FORMAT_R8G8B8A8_UNORM;

	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable	= false;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DSVFormat;

	//���X�^���C�U
	gpsDesc.RasterizerState				= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;
	gpsDesc.RasterizerState.FillMode	= D3D12_FILL_MODE_SOLID;

	//���̑�
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

void Wrapper::InitConstants()
{
	auto wsize = Application::GetInstance().GetWIndowSize();

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = _dev->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_rgstDescHeap));

	//�J�����ݒ�
	XMFLOAT3 eye(0, 15, -15);//���_
	XMFLOAT3 target(0, 10, 0);//�����_
	XMFLOAT3 up(0, 10, 10);//��x�N�g��

	//�x�N�g���ϊ�
	XMVECTOR veye = XMLoadFloat3(&eye);

	//����n,�r���[�s��
	_wvp._view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	//�v���W�F�N�V�����s��
	//��p�A�A�X�y�N�g��A�j�A�A�t�@�[
	_wvp._projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,
		static_cast<float>(wsize.w) / static_cast<float>(wsize.h),
		0.1f,
		300);

	//���[���h�s��
	angle = 0.0f;
	_wvp._world = XMMatrixRotationY(angle);

	size_t size = sizeof(_wvp);
	size = (size + 0xff)&~0xff;
	//size = size+(256 - size % 256)% 256
	//���s����256�r�b�g���Ƃɐ�������

	//�R���X�^���g�o�b�t�@
	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_cBuff));

	//�}�b�v
	XMMATRIX* mat = nullptr;
	result = _cBuff->Map(0, nullptr, (void**)&_mappedWvp);
	memcpy(_mappedWvp, &_wvp, sizeof(_wvp));
	_cBuff->Unmap(0, nullptr);

	//�萔�o�b�t�@�r���[�쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _cBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;

	_dev->CreateConstantBufferView(&cbvDesc, _rgstDescHeap->GetCPUDescriptorHandleForHeapStart());

	anglex = 0.0f;
	angley = 0.0f;
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

	//�R�}���h�̃��Z�b�g
	auto result = _cmdAllocator->Reset();
	result = _cmdList->Reset(_cmdAllocator, nullptr);

	//�p�C�v���C���̃Z�b�g
	_cmdList->SetPipelineState(_pipeline);

	//���[�g�V�O�l�`���̃Z�b�g
	_cmdList->SetGraphicsRootSignature(_rootSignature);

	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	//�o���A�Z�b�g
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbidx],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);

	//�N���A
	_cmdList->ClearRenderTargetView(heapStart,clearColor,0, nullptr);

	//�e�N�X�`���Z�b�g
	_cmdList->SetDescriptorHeaps(1, &_texsrvHeap);
	_cmdList->SetGraphicsRootDescriptorTable(0, _texsrvHeap->GetGPUDescriptorHandleForHeapStart());

	//���_�Z�b�g
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_cmdList->IASetVertexBuffers(0, 1, &_vbView);

	_cmdList->IASetIndexBuffer(&_idxbView);
	_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	
	//�o���A��
	_cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbidx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	_cmdList->Close();

	ExecuteCmd();

	//�҂�
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
