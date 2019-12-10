#include "Camera.h"
#include "Application.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>

//�����N
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

	//�J�����ݒ�
	eye = XMFLOAT3(0, 18, -50);//���_
	target = XMFLOAT3(0, 10, 0);//�����_
	up = XMFLOAT3(0, 1, 0);//��x�N�g��

	XMFLOAT3 light(-10, 20, -10);
	XMMATRIX lightview = XMMatrixLookAtLH(XMLoadFloat3(&light),XMLoadFloat3(&target),XMLoadFloat3(&up));
	XMMATRIX lightproj = XMMatrixOrthographicLH(200, 200, 0.1f, 500.0f);

	_wvp._lvp = lightview * lightproj;

	//�x�N�g���ϊ�
	//XMVECTOR veye = XMLoadFloat3(&eye);

	//����n,�r���[�s��
	_wvp._view = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), 
		XMLoadFloat3(&target), 
		XMLoadFloat3(&up));

	//�v���W�F�N�V�����s��
	//��p�A�A�X�y�N�g��A�j�A�A�t�@�[
	_wvp._projection = 
		XMMatrixPerspectiveFovLH(
		XM_PIDIV2 / 3,
		static_cast<float>(wsize.w) / static_cast<float>(wsize.h),
		1.0f,
		300);

	//���[���h�s��
	angle = 0.0f;
	_wvp._world = XMMatrixRotationY(angle);

	size_t size = sizeof(_wvp);
	size = (size + 0xff)&~0xff;
	//size = size+(256 - size % 256)% 256
	//���s����256�r�b�g���Ƃɐ�������

	//�R���X�^���g�o�b�t�@
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_cBuff));

	//�}�b�v
	result = _cBuff->Map(0, nullptr, (void**)&_mappedWvp);
	memcpy(_mappedWvp, &_wvp, sizeof(_wvp));
	_cBuff->Unmap(0, nullptr);

	//�萔�o�b�t�@�r���[�쐬
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
		//�J������]
		if (keyState[VK_LEFT] & 0x80)
		{
			anglex += 0.01f;
		}
		if (keyState[VK_RIGHT] & 0x80)
		{
			anglex += -0.01f;
		}
		/*if (keyState[VK_UP] & 0x80)
		{
			angley += 0.01f;
		}
		if (keyState[VK_DOWN] & 0x80)
		{
			angley += -0.01f;
		}*/

		//�J�����ړ�
		if (keyState['W'] & 0x80)
		{
			eye.z++;
			target.z++;
		}
		if (keyState['S'] & 0x80)
		{
			eye.z--;
			target.z--;
		}
		if (keyState['A'] & 0x80)
		{
			target.x--;
			eye.x--;
		}
		if (keyState['D'] & 0x80)
		{
			target.x++;
			eye.x++;
		}
		if (keyState[VK_UP] & 0x80)
		{
			target.y++;
			eye.y++;
		}
		if (keyState[VK_DOWN] & 0x80)
		{
			target.y--;
			eye.y--;
		}

		//���C�g��]
		if (keyState['L'] & 0x80)
		{
			lightanglex = 0.01f;

			_wvp._lvp = DirectX::XMMatrixRotationY(lightanglex) * _wvp._lvp;		//���C�g��]
		}
		if (keyState['J'] & 0x80)
		{
			lightanglex = -0.01f;

			_wvp._lvp = DirectX::XMMatrixRotationY(lightanglex) * _wvp._lvp;		//���C�g��]
		}
	}

	_wvp._view = XMMatrixLookAtLH(
			XMLoadFloat3(&eye),
			XMLoadFloat3(&target),
			XMLoadFloat3(&up));

	//auto ray = XMLoadFloat3(&target) - XMLoadFloat3(&eye);
	//auto right = XMVector3Cross(XMLoadFloat3(&up), ray);

	////�E�x�N�g�����ɉ�]
	//_wvp._view *= XMMatrixRotationQuaternion(XMQuaternionRotationAxis(right, angley));

	//auto upper = XMVector3Cross(ray, right);

	////��x�N�g�����ɉ�]
	//_wvp._view *= XMMatrixRotationQuaternion(XMQuaternionRotationAxis(upper, anglex));

	_wvp._view = DirectX::XMMatrixRotationY(anglex) * _wvp._view;	// ��]
	_wvp._view = DirectX::XMMatrixRotationX(angley) * _wvp._view;	// ��]
	
	_wvp._wvp = _wvp._world;
	_wvp._wvp *= _wvp._view;
	_wvp._wvp *= _wvp._projection;
	*_mappedWvp = _wvp;
}

