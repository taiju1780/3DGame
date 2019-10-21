#include "PMXModel.h"
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

using namespace DirectX;

namespace {

	std::string GetExtension(const std::string& modelpath) {
		int idx = modelpath.rfind('.');
		return modelpath.substr(idx + 1, modelpath.length() - idx - 1);
	}

	std::pair<std::string, std::string>
		SplitFileName(const std::string& path, const char split = '*') {
		int idx = path.find(split);
		std::pair<std::string, std::string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);
		return ret;
	}

	std::string GetTexPath(const char* modelpath) {
		std::string _Modelpath = modelpath;

		auto idx1 = _Modelpath.rfind("/");
		auto idx2 = _Modelpath.rfind("\\");
		idx2 = std::string::npos ? 0 : idx2;
		auto idx = max(idx1, idx2);
		auto pathIndex = _Modelpath.substr(0, idx) + "/";

		return pathIndex;
	}

	std::string GetTexAstPath(const char* modelpath) {
		std::string _Modelpath = modelpath;

		auto splittercount = std::count(_Modelpath.begin(), _Modelpath.end(), '*');

		if (splittercount > 0) {
			auto namepair = SplitFileName(_Modelpath);
			if (GetExtension(namepair.first) == "sph" ||
				GetExtension(namepair.first) == "spa") {
				_Modelpath = namepair.second;
			}
			else {
				_Modelpath = namepair.first;
			}
		}
		return _Modelpath;
	}
}

void PMXModel::LoadModel(const char * filepath, ID3D12Device* _dev)
{
	FILE* fp;
	fopen_s(&fp, filepath, "rb");

	PMXHeader pmxH = {};
	fread(&pmxH.magic, sizeof(pmxH.magic), 1, fp);
	fread(&pmxH.version, sizeof(pmxH.version), 1, fp);
	fread(&pmxH.bitesize, sizeof(pmxH.bitesize), 1, fp);
	fread(&pmxH.data, sizeof(pmxH.data), 1, fp);

	ModelInfo modelI = {};
	fread(&modelI.ModelNamesize, sizeof(modelI.ModelNamesize), 1, fp);
	fseek(fp, modelI.ModelNamesize, SEEK_CUR);

	fread(&modelI.ModelNameEsize, sizeof(modelI.ModelNameEsize), 1, fp);
	fseek(fp, modelI.ModelNameEsize, SEEK_CUR);

	fread(&modelI.Commentsize, sizeof(modelI.Commentsize), 1, fp);
	fseek(fp, modelI.Commentsize, SEEK_CUR);

	fread(&modelI.CommentEsize, sizeof(modelI.CommentEsize), 1, fp);
	fseek(fp, modelI.CommentEsize, SEEK_CUR);

	unsigned int Vnum = 0;
	fread(&Vnum, sizeof(Vnum), 1, fp);

	_verticesData.resize(Vnum);

	//waitclassの値
	int adduvnum = pmxH.data[1];
	int vertidxsize = pmxH.data[2];
	int texidxsize = pmxH.data[3];
	int bornidxsize = pmxH.data[5];

	for (auto i = 0; i < Vnum; ++i) {
		//ポジション
		fread(&_verticesData[i].pos, sizeof(_verticesData[i].pos), 1, fp);

		//法線
		fread(&_verticesData[i].normal, sizeof(_verticesData[i].normal), 1, fp);

		//UV
		fread(&_verticesData[i].uv, sizeof(_verticesData[i].uv), 1, fp);

		//追加uv
		for (int u = 0; u < adduvnum; ++u) {
			fread(&_verticesData[i].addUv[u], sizeof(_verticesData[i].addUv[u]), 1, fp);
		}
		
		//waitclass
		fread(&_verticesData[i].waitclass, sizeof(_verticesData[i].waitclass), 1, fp);
		
		int waitnum = _verticesData[i].waitclass;

		//ボーン
		if (waitnum == 0) {
			fread(&_verticesData[i].bone[0], bornidxsize, 1, fp);
		}
		else if (waitnum == 1) {
			
			fread(&_verticesData[i].bone[0], bornidxsize, 1, fp);
			fread(&_verticesData[i].bone[1], bornidxsize, 1, fp);
			fread(&_verticesData[i].wait[0], sizeof(_verticesData[i].wait[0]), 1, fp);
		}
		else if (waitnum == 2) {
			for (int bw = 0; bw < 4; ++bw) {
				fread(&_verticesData[i].bone[bw], bornidxsize, 1, fp);
				fread(&_verticesData[i].wait[bw], sizeof(_verticesData[i].wait[bw]), 1, fp);
			}
		}
		else if (waitnum == 3) {
			fread(&_verticesData[i].bone[0], bornidxsize, 1, fp);
			fread(&_verticesData[i].bone[1], bornidxsize, 1, fp);
			fread(&_verticesData[i].wait[0], sizeof(_verticesData[i].wait[0]), 1, fp);
			for (int s = 0; s < 3; ++s) {
				fread(&_verticesData[i].sdefvec[s], sizeof(_verticesData[i].sdefvec[s]), 1, fp);
			}
		}
		fread(&_verticesData[i].edge, sizeof(_verticesData[i].edge), 1, fp);
	}

	unsigned int idxNum = 0;
	fread(&idxNum, sizeof(idxNum), 1, fp);

	_indexData.resize(idxNum);

	for (int i = 0; i < idxNum; ++i) {
		fread(&_indexData[i], vertidxsize, 1, fp);
	}

	unsigned int textureNum;
	fread(&textureNum, sizeof(textureNum), 1, fp);
	_texpath.resize(textureNum);

	for (int t = 0; t < textureNum; ++t) {

		unsigned int pathlength = 0;
		fread(&pathlength, sizeof(pathlength), 1, fp);

		std::string str;

		for (int i = 0; i < pathlength /2; ++i) {

			wchar_t c;
			fread(&c, sizeof(wchar_t), 1, fp);
			str += c;
		}
		_texpath[t] = str;
	}

	unsigned int MatNum = 0;
	fread(&MatNum, sizeof(MatNum), 1, fp);

	_matData.resize(MatNum);

	for (auto &mat : _matData) {
		unsigned int bytenum = 0;
		fread(&bytenum, sizeof(bytenum), 1, fp);
		fseek(fp, bytenum, SEEK_CUR);

		fread(&bytenum, sizeof(bytenum), 1, fp);
		fseek(fp, bytenum, SEEK_CUR);

		fread(&mat.diffuse, sizeof(mat.diffuse), 1, fp);
		fread(&mat.specular, sizeof(mat.specular), 1, fp);
		fread(&mat.specPower, sizeof(mat.specPower), 1, fp);
		fread(&mat.ambient, sizeof(mat.ambient), 1, fp);

		fread(&mat.bitFlag, sizeof(mat.bitFlag), 1, fp);
		fread(&mat.edgeColer, sizeof(mat.edgeColer), 1, fp);
		fread(&mat.edgesize, sizeof(mat.edgesize), 1, fp);

		fread(&mat.textureIndex, texidxsize, 1, fp);
		fread(&mat.sphIndex, texidxsize, 1, fp);

		fread(&mat.sphmode, sizeof(mat.sphmode), 1, fp);
		fread(&mat.toonflag, sizeof(mat.toonflag), 1, fp);

		if (mat.toonflag == 0) {
			fread(&mat.toonidx, texidxsize, 1, fp);
		}
		else {
			fread(&mat.toonidx, sizeof(unsigned char), 1, fp);
		}

		fread(&bytenum, sizeof(bytenum), 1, fp);
		fseek(fp, bytenum, SEEK_CUR);

		fread(&mat.face_vert_cnt, sizeof(mat.face_vert_cnt), 1, fp);
	}

	fclose(fp);

	size_t idx = 0;
	_texturePaths.resize(MatNum);
	auto folder = GetTexPath(filepath);

	for (int i = 0; i < MatNum;++i) {
		if (_matData[i].toonidx != 0xff) {
			if (_matData[i].toonflag == 1) {
				toonfilepath = GetToonTexpathFromIndex(_matData[i].toonidx, folder);
				InitToon(toonfilepath, _dev, idx);
			}
			{
				InitToon("", _dev, idx);
			}
		}
		else {
			InitToon("", _dev, idx);
		}
		if (std::strlen(_texpath[i].c_str()) > 0) {
			_texturePaths[idx] = folder + GetTexAstPath(_texpath[i].c_str());
		}
		idx++;
	}
}

void PMXModel::CreatModelTex(ID3D12Device * _dev)
{
	auto& _texturepath = _texturePaths;
	auto& pMat = _matData;

	_TexBuff.resize(_texturepath.size());
	_TexBuffspa.resize(_texturepath.size());
	_TexBuffsph.resize(_texturepath.size());

	for (int i = 0; i < _texturepath.size(); i++) {

		TexMetadata metadata = {};
		ScratchImage scratchimg = {};

		_TexBuff[i] = _whiteTexbuff;
		_TexBuffsph[i] = _whiteTexbuff;
		_TexBuffspa[i] = _blackTexbuff;

		if (_texturepath[i] == "")continue;

		auto texpoint = _texturepath[i].rfind(".");
		auto texpointExt = _texturepath[i].substr(texpoint + 1);
		auto texpath = StringToWStirng(_texturepath[i]);

		if ((texpointExt == "sph") || (texpointExt == "spa") ||
			(texpointExt == "jpg") || (texpointExt == "png") || (texpointExt == "bmp")) {
			auto result = LoadFromWICFile(
				texpath.c_str(),
				WIC_FLAGS_NONE,
				&metadata,
				scratchimg);
		}
		else if (texpointExt == "tga") {
			auto result = LoadFromTGAFile(
				texpath.c_str(),
				&metadata,
				scratchimg);
		}
		else if (texpointExt == "dds") {
			auto result = LoadFromDDSFile(
				texpath.c_str(),
				WIC_FLAGS_NONE,
				&metadata,
				scratchimg);
		}

		D3D12_HEAP_PROPERTIES heapprop = {};
		heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
		heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
		heapprop.CreationNodeMask = 0;
		heapprop.VisibleNodeMask = 0;

		//テクスチャデスク
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Alignment = 0;									//先頭からなので0
		texDesc.DepthOrArraySize = metadata.arraySize;					//リソースが2Dで配列でもないので１
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//何次元テクスチャか(TEXTURE2D)
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;				//NONE
		texDesc.Format = metadata.format;						//例によって
		texDesc.Width = metadata.width;						//テクスチャ幅
		texDesc.Height = metadata.height;						//テクスチャ高さ
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;			//決定できないのでUNKNOWN
		texDesc.MipLevels = metadata.mipLevels;					//ミップ使ってないので0
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;

		ID3D12Resource* tmpbuff = nullptr;

		auto result = _dev->CreateCommittedResource(
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

		if (texpointExt == "sph") {
			_TexBuffsph[i] = tmpbuff;
		}
		else if (texpointExt == "spa") {
			_TexBuffspa[i] = tmpbuff;
		}
		else {
			_TexBuff[i] = tmpbuff;
		}

		scratchimg.Release();
	}
}

void PMXModel::InitMaterial(ID3D12Device * _dev)
{
	auto pMat = _matData;

	_matBuffs.resize(pMat.size());
	size_t size = sizeof(Material);
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
		mappedColor->diffuse_color.x = pMat[idx].diffuse.x;
		mappedColor->diffuse_color.y = pMat[idx].diffuse.y;
		mappedColor->diffuse_color.z = pMat[idx].diffuse.z;

		//アンビエント
		mappedColor->ambient.x = pMat[idx].ambient.x;
		mappedColor->ambient.y = pMat[idx].ambient.y;
		mappedColor->ambient.z = pMat[idx].ambient.z;

		//スペキュラー
		mappedColor->specular_color.x = pMat[idx].specular.x;
		mappedColor->specular_color.y = pMat[idx].specular.y;
		mappedColor->specular_color.z = pMat[idx].specular.z;

		m->Unmap(0, nullptr);

		idx++;
	}

	//マテリアルのヒープ
	D3D12_DESCRIPTOR_HEAP_DESC matHeapDesc = {};
	matHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matHeapDesc.NodeMask = 0;
	matHeapDesc.NumDescriptors = pMat.size() * 5;
	matHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&matHeapDesc, IID_PPV_ARGS(&_matHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC matViewDesc = {};
	auto matH = _matHeap->GetCPUDescriptorHandleForHeapStart();

	for (auto i = 0; i < pMat.size(); ++i) {
		//定数バッファ
		matViewDesc.BufferLocation = _matBuffs[i]->GetGPUVirtualAddress();
		matViewDesc.SizeInBytes = size;
		_dev->CreateConstantBufferView(&matViewDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//テクスチャのヒープ
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		//t1
		srvDesc.Format = _TexBuff[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(_TexBuff[i], &srvDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//t2のシェーダリソースビュー(sph)
		srvDesc.Format = _TexBuffsph[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(_TexBuffsph[i], &srvDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//t3のシェーダリソースビュー(spa)
		srvDesc.Format = _TexBuffspa[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(_TexBuffspa[i], &srvDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//t4のシェーダリソースビュー(toon)
		srvDesc.Format = _ToonBuff[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(_ToonBuff[i], &srvDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

std::string PMXModel::GetToonTexpathFromIndex(int idx, std::string folderpath) {
	std::string filepath = toonTexNames[idx];
	std::string toonpath = "toon/";
	toonpath += filepath;
	if (PathFileExists(toonpath.c_str())) {
		return toonpath;
	}
	else {
		return folderpath + filepath;
	}
}

void PMXModel::InitToon(std::string path, ID3D12Device * _dev, size_t idx) {

	_ToonBuff.resize(_matData.size());

	TexMetadata metadata = {};
	ScratchImage scratchimg = {};

	_ToonBuff[idx] = _gladTexBuff;

	if (toonfilepath != "") {
		auto toonpath = StringToWStirng(toonfilepath);

		auto result = LoadFromWICFile(
			toonpath.c_str(),
			WIC_FLAGS_NONE,
			&metadata,
			scratchimg);

		D3D12_HEAP_PROPERTIES heapprop = {};
		heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
		heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
		heapprop.CreationNodeMask = 0;
		heapprop.VisibleNodeMask = 0;

		//テクスチャデスク
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Alignment = 0;									//先頭からなので0
		texDesc.DepthOrArraySize = metadata.arraySize;					//リソースが2Dで配列でもないので１
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//何次元テクスチャか(TEXTURE2D)
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;				//NONE
		texDesc.Format = metadata.format;						//例によって
		texDesc.Width = metadata.width;						//テクスチャ幅
		texDesc.Height = metadata.height;						//テクスチャ高さ
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;			//決定できないのでUNKNOWN
		texDesc.MipLevels = metadata.mipLevels;					//ミップ使ってないので0
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;

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

		_ToonBuff[idx] = tmpbuff;

		scratchimg.Release();
	}
}

std::vector<PMXVertex> PMXModel::GetverticesData()
{
	return _verticesData;
}

std::vector<unsigned short> PMXModel::GetindexData()
{
	return _indexData;
}

std::vector<Material> PMXModel::GetmatData()
{
	return _matData;
}

ID3D12DescriptorHeap*& PMXModel::GetMatHeap()
{
	return _matHeap;
}

void PMXModel::CreateWhiteTexture(ID3D12Device* _dev)
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

void PMXModel::CreateBlackTexture(ID3D12Device* _dev)
{
	D3D12_HEAP_PROPERTIES Wheapprop = {};
	Wheapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	Wheapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	Wheapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	Wheapprop.CreationNodeMask = 1;
	Wheapprop.VisibleNodeMask = 1;

	//テクスチャデスク
	D3D12_RESOURCE_DESC WtexDesc = {};
	WtexDesc.Alignment = 0;										//先頭からなので0
	WtexDesc.DepthOrArraySize = 1;										//リソースが2Dで配列でもないので１
	WtexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//何次元テクスチャか(TEXTURE2D)
	WtexDesc.Flags = D3D12_RESOURCE_FLAG_NONE;					//NONE
	WtexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//例によって
	WtexDesc.Width = 4;										//テクスチャ幅
	WtexDesc.Height = 4;										//テクスチャ高さ
	WtexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;				//決定できないのでUNKNOWN
	WtexDesc.MipLevels = 1;										//ミップ使ってないので0
	WtexDesc.SampleDesc.Count = 1;
	WtexDesc.SampleDesc.Quality = 0;

	auto result = _dev->CreateCommittedResource(
		&Wheapprop,
		D3D12_HEAP_FLAG_NONE,
		&WtexDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_blackTexbuff));

	//黒テクスチャ
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);
	result = _blackTexbuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, 4 * 4 * 4);
}

void PMXModel::CreateGraduation(ID3D12Device* _dev)
{
	size_t size = sizeof(float) * 256;
	//size = (size + 0xff)&~0xff;

	CD3DX12_HEAP_PROPERTIES hProp = {};
	hProp.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
	hProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	hProp.CreationNodeMask = 1;
	hProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	hProp.VisibleNodeMask = 1;

	CD3DX12_RESOURCE_DESC rDesc = {};
	rDesc.Height = 256;
	rDesc.Width = 4;
	rDesc.SampleDesc.Count = 1;
	rDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	rDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	rDesc.DepthOrArraySize = 1;
	rDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rDesc.MipLevels = 1;

	auto result = _dev->CreateCommittedResource(
		&hProp,
		D3D12_HEAP_FLAG_NONE,
		&rDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_gladTexBuff));

	struct Color {
		unsigned char r, g, b, a;
		Color() :r(0), g(0), b(0), a(0) {};
		Color(unsigned char inr, unsigned char ing, unsigned char inb, unsigned char ina)
			:r(inr), g(ing), b(inb), a(ina) {};
	};

	std::vector<Color> cdata(4 * 256);

	auto itr = cdata.begin();

	unsigned char brightness(255);

	for (; itr != cdata.end(); itr += 4) {
		//00000000111111112222222233333333、みたいな感じのをつくる
		std::fill_n(itr, 4, Color(brightness, brightness, brightness, 0xff));
		--brightness;
	}

	result = _gladTexBuff->WriteToSubresource
	(0, nullptr, cdata.data(), 4 * sizeof(Color), cdata.size() * sizeof(Color));
}

std::wstring PMXModel::StringToWStirng(const std::string& str) {
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

PMXModel::PMXModel(const char * filepath,ID3D12Device* _dev)
{
	CreateGraduation(_dev);
	LoadModel(filepath, _dev);
	CreateWhiteTexture(_dev);
	CreateBlackTexture(_dev);
	CreatModelTex(_dev);
	InitMaterial(_dev);
}


PMXModel::~PMXModel()
{
}
