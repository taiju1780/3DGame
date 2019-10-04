#include "PMDModel.h"
#include <iostream>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <Windows.h>
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <vector>
#include<Shlwapi.h> 

//リンク
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment (lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"shlwapi.lib")


unsigned int MatSizeFlont = 46;
unsigned int MatSizeBack = 24;

PMDModel::PMDModel(const char * filepath, ID3D12Device* _dev)
{
	InitModel(filepath);
	InitMaterial(_dev);
}


PMDModel::~PMDModel()
{
}

std::vector<PMDvertex> PMDModel::GetverticesData()
{
	return _verticesData;
}

std::vector<unsigned short> PMDModel::GetindexData()
{
	return _indexData;
}

std::vector<PMDMaterial> PMDModel::GetmatData()
{
	return _matData;
}

ID3D12DescriptorHeap*& PMDModel::GetMatHeap() 
{
	return _matHeap;
}

void PMDModel::InitModel(const char * filepath)
{
	FILE *fp;

	fopen_s(&fp, filepath, "rb");

	//ヘッダ読み込み
	PMDHeader pmdheader = {};
	fread(&pmdheader.magic, sizeof(pmdheader.magic), 1, fp);
	fread(&pmdheader.version, sizeof(pmdheader) - sizeof(pmdheader.magic) - 1, 1, fp);

	//頂点
	unsigned int Vnum = 0;
	fread(&Vnum, sizeof(Vnum), 1, fp);

	_verticesData.resize(Vnum);

	for (auto i = 0; i < Vnum; ++i) {
		fread(&_verticesData[i], sizeof(PMDvertex), 1, fp);
	}

	//インデックス
	unsigned int IdxNum = 0;
	fread(&IdxNum, sizeof(IdxNum), 1, fp);

	_indexData.resize(IdxNum);
	for (auto i = 0; i < IdxNum; ++i) {
		fread(&_indexData[i], sizeof(unsigned short), 1, fp);
	}

	//マテリアル
	unsigned int matNum = 0;
	fread(&matNum, sizeof(matNum), 1, fp);

	_matData.resize(matNum);
	for (auto &mat : _matData) {
		fread(&mat, MatSizeFlont, 1, fp);
		fread(&mat.face_vert_count, MatSizeBack, 1, fp);
	}
}

void PMDModel::InitMaterial(ID3D12Device * _dev)
{
	auto pMat = _matData;

	_matBuffs.resize(pMat.size());
	size_t size = sizeof(PMDMaterial);
	size = (size + 0xff)&~0xff;

	size_t idx = 0;
	for (auto &m : _matBuffs) {
		_dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m)
		);

		m->Map(0, nullptr, (void**)&mappedColor);

		//ディフューズ
		mappedColor->diffuse_color.x = pMat[idx].diffuse_color.x;
		mappedColor->diffuse_color.y = pMat[idx].diffuse_color.y;
		mappedColor->diffuse_color.z = pMat[idx].diffuse_color.z;
		mappedColor->diffuse_color.w = pMat[idx].alpha;

		//アンビエント
		mappedColor->ambient.x = pMat[idx].mirror_color.x;
		mappedColor->ambient.y = pMat[idx].mirror_color.y;
		mappedColor->ambient.z = pMat[idx].mirror_color.z;

		//スペキュラー
		mappedColor->specular_color.x = pMat[idx].specular_color.x;
		mappedColor->specular_color.y = pMat[idx].specular_color.y;
		mappedColor->specular_color.z = pMat[idx].specular_color.z;
		mappedColor->specular_color.w = pMat[idx].specular;

		m->Unmap(0, nullptr);

		idx++;
	}

	D3D12_DESCRIPTOR_HEAP_DESC matHeapDesc = {};
	matHeapDesc.Flags					= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matHeapDesc.NodeMask				= 0;
	matHeapDesc.NumDescriptors			= pMat.size();;
	matHeapDesc.Type					= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&matHeapDesc, IID_PPV_ARGS(&_matHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC matViewDesc = {};
	auto matH = _matHeap->GetCPUDescriptorHandleForHeapStart();

	for (auto i = 0; i < pMat.size(); ++i) {
		//定数バッファ
		matViewDesc.BufferLocation = _matBuffs[i]->GetGPUVirtualAddress();
		matViewDesc.SizeInBytes = size;
		_dev->CreateConstantBufferView(&matViewDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

std::wstring PMDModel::StringToWStirng(const std::string& str) {
	auto ssize = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.data(),
		str.length(),
		nullptr,
		0
	);

	std::wstring wstr;
	wstr.resize(ssize);

	ssize = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.data(),
		str.length(),
		&wstr[0],
		ssize);
	assert(ssize == wstr.length());
	return wstr;
}
