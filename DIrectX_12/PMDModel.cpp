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

using namespace DirectX;

namespace {
	std::string GetTexPath(const char* modelpath) {
		std::string _Modelpath;
		auto idx1 = _Modelpath.rfind("/");
		auto idx2 = _Modelpath.rfind("\\");
		idx2 = std::string::npos ? 0 : idx2;
		auto idx = max(idx1, idx2);
		auto pathIndex = _Modelpath.substr(0, idx) + "/";
		return pathIndex;
	}
}

void PMDModel::CreatModelTex(ID3D12Device * _dev)
{
	auto& _texturepath = _texturePaths;
	auto& pMat = _matData;

	_TexBuff.resize(_texturepath.size());

	for (auto i = 0; 0 < _texturepath.size(); ++i) {

		TexMetadata metadata = {};
		ScratchImage scratchimg = {};

		_TexBuff[i] = _whiteTexbuff;

		if (_texturepath[i] == "")continue;

		auto texpath = StringToWStirng(_texturepath[i]);
		auto folder = "Model/";

		auto result = LoadFromWICFile(
			texpath.c_str(),
			WIC_FLAGS_NONE,
			&metadata,
			scratchimg);

		D3D12_HEAP_PROPERTIES heapprop = {};
		heapprop.Type					= D3D12_HEAP_TYPE_CUSTOM;
		heapprop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		heapprop.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
		heapprop.CreationNodeMask		= 1;
		heapprop.VisibleNodeMask		= 1;

		//テクスチャデスク
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Alignment			= 0;									//先頭からなので0
		texDesc.DepthOrArraySize	= metadata.arraySize;					//リソースが2Dで配列でもないので１
		texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//何次元テクスチャか(TEXTURE2D)
		texDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;				//NONE
		texDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;			//例によって
		texDesc.Width				= metadata.width;						//テクスチャ幅
		texDesc.Height				= metadata.height;						//テクスチャ高さ
		texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;			//決定できないのでUNKNOWN
		texDesc.MipLevels			= metadata.mipLevels;					//ミップ使ってないので0
		texDesc.SampleDesc.Count	= 1;
		texDesc.SampleDesc.Quality	= 0;

		ID3D12Resource* tmpbuff = nullptr;

		result = _dev->CreateCommittedResource(
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&tmpbuff));

		result = tmpbuff->WriteToSubresource(
			0,
			nullptr,
			scratchimg.GetPixels(),
			metadata.width * 4,
			scratchimg.GetPixelsSize());

		_TexBuff[i] = tmpbuff;

		scratchimg.Release();
	}
}

void PMDModel::CreateWhiteTexture(ID3D12Device* _dev)
{
	D3D12_HEAP_PROPERTIES Wheapprop = {};
	Wheapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	Wheapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	Wheapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	Wheapprop.CreationNodeMask = 1;
	Wheapprop.VisibleNodeMask = 1;

	//テクスチャデスク
	D3D12_RESOURCE_DESC WtexDesc = {};
	WtexDesc.Alignment = 0;									//先頭からなので0
	WtexDesc.DepthOrArraySize = 1;									//リソースが2Dで配列でもないので１
	WtexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//何次元テクスチャか(TEXTURE2D)
	WtexDesc.Flags = D3D12_RESOURCE_FLAG_NONE;			//NONE
	WtexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			//例によって
	WtexDesc.Width = 4;									//テクスチャ幅
	WtexDesc.Height = 4;									//テクスチャ高さ
	WtexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;		//決定できないのでUNKNOWN
	WtexDesc.MipLevels = 1;									//ミップ使ってないので0
	WtexDesc.SampleDesc.Count = 1;
	WtexDesc.SampleDesc.Quality = 0;

	auto result = _dev->CreateCommittedResource(
		&Wheapprop,
		D3D12_HEAP_FLAG_NONE,
		&WtexDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_whiteTexbuff));

	//白テクスチャ
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);
	result = _whiteTexbuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, 4 * 4 * 4);
}

PMDModel::PMDModel(const char * filepath, ID3D12Device* _dev)
{
	InitModel(filepath);
	CreateWhiteTexture(_dev);
	CreatModelTex(_dev);
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

	size_t idx = 0;
	_texturePaths.resize(matNum);
	auto folder = GetTexPath(filepath);

	for (auto &mat : _matData) {
		if (std::strlen(mat.texture_file_name) > 0) {
			_texturePaths[idx] = folder + mat.texture_file_name;
		}
		idx++;
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

	//マテリアルのヒープ
	D3D12_DESCRIPTOR_HEAP_DESC matHeapDesc = {};
	matHeapDesc.Flags					= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matHeapDesc.NodeMask				= 0;
	matHeapDesc.NumDescriptors			= pMat.size() * 2;
	matHeapDesc.Type					= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&matHeapDesc, IID_PPV_ARGS(&_matHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC matViewDesc = {};
	auto matH = _matHeap->GetCPUDescriptorHandleForHeapStart();

	//テクスチャのヒープ
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format							= DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels				= 1;

	for (auto i = 0; i < pMat.size(); ++i) {
		//定数バッファ
		matViewDesc.BufferLocation	= _matBuffs[i]->GetGPUVirtualAddress();
		matViewDesc.SizeInBytes		= size;
		_dev->CreateConstantBufferView(&matViewDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//t1
		_dev->CreateShaderResourceView(_TexBuff[i], &srvDesc, matH);
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
