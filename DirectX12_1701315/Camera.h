#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>

//ワールドビュープロジェクション
struct WVPMatrix {
	DirectX::XMMATRIX _world;
	DirectX::XMMATRIX _view;
	DirectX::XMMATRIX _projection;
	DirectX::XMMATRIX _wvp;
	DirectX::XMMATRIX _lvp;				//float16：ライトビュー行列
};

class Camera
{
private:
	void InitConstants(ID3D12Device* dev);
	ID3D12Resource* _cBuff;
	ID3D12DescriptorHeap* _rgstDescHeap = nullptr;

	//マトリクス関係
	float posx;
	float posy;
	float posz;

	float angle;
	float anglex;
	float angley;

	WVPMatrix _wvp;
	WVPMatrix* _mappedWvp;
	DirectX::XMFLOAT3 eye; 
	DirectX::XMFLOAT3 target;
	DirectX::XMFLOAT3 up;

public:
	Camera(ID3D12Device* dev);
	~Camera();
	ID3D12DescriptorHeap*& GetrgstDescHeap();
	void CameraUpdate(unsigned char keyState[]);
};

