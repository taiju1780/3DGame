#include "Camera.h"
#include "Application.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>

//リンク
#pragma comment(lib,"d3d12.lib") 
#pragma comment(lib,"dxgi.lib") 
#pragma comment (lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;

Camera::Camera(ID3D12Device* dev)
{
	InitConstants(dev);
}


Camera::~Camera()
{
}

ID3D12DescriptorHeap *& Camera::GetrgstDescHeap()
{
	return _rgstDescHeap;
}

void Camera::InitConstants(ID3D12Device* dev)
{
	auto wsize = Application::GetInstance().GetWIndowSize();

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = dev->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_rgstDescHeap));

	//カメラ設定
	XMFLOAT3 eye(0, 18, -15);//視点
	XMFLOAT3 target(0, 12, 0);//注視点
	XMFLOAT3 up(0, 1, 0);//上ベクトル

	//ベクトル変換
	//XMVECTOR veye = XMLoadFloat3(&eye);

	//左手系,ビュー行列
	_wvp._view = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), 
		XMLoadFloat3(&target), 
		XMLoadFloat3(&up));

	//プロジェクション行列
	//画角、アスペクト比、ニア、ファー
	_wvp._projection = 
		XMMatrixPerspectiveFovLH(
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
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_cBuff));

	//マップ
	result = _cBuff->Map(0, nullptr, (void**)&_mappedWvp);
	memcpy(_mappedWvp, &_wvp, sizeof(_wvp));
	_cBuff->Unmap(0, nullptr);

	//定数バッファビュー作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _cBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;

	dev->CreateConstantBufferView(&cbvDesc, _rgstDescHeap->GetCPUDescriptorHandleForHeapStart());

	anglex = 0.0f;
	angley = 0.0f;
}

void Camera::CameraUpdate(unsigned char keyState[])
{
	if (GetKeyboardState(keyState)) {
		//カメラ回転
		if (keyState['A'] & 0x80)
		{
			anglex = 0.005f;
			_wvp._view = DirectX::XMMatrixRotationY(anglex) * _wvp._view;	// 回転
		}
		if (keyState['D'] & 0x80)
		{
			anglex = -0.005f;
			_wvp._view = DirectX::XMMatrixRotationY(anglex) * _wvp._view;	// 回転
		}

		//座標回転
		if (keyState['L'] & 0x80)
		{
			anglex = 0.005f;
			_wvp._world = DirectX::XMMatrixRotationY(anglex) * _wvp._world;
		}
		if (keyState['J'] & 0x80)
		{
			anglex = -0.005f;
			_wvp._world = DirectX::XMMatrixRotationY(anglex) * _wvp._world;	
		}
	}
	

	_wvp._wvp = _wvp._world;
	_wvp._wvp *= _wvp._view;
	_wvp._wvp *= _wvp._projection;
	*_mappedWvp = _wvp;
}
