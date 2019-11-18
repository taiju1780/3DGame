#include "PMXModel.h"
#include "Application.h"
#include <iostream>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <Windows.h>
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <vector>
#include<Shlwapi.h> 
#include "Wrapper.h"

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
	int matidxsize = pmxH.data[3];
	int boneidxsize = pmxH.data[5];
	int morphidxsize = pmxH.data[6];

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
			fread(&_verticesData[i].bone[0], boneidxsize, 1, fp);
		}
		else if (waitnum == 1) {
			
			fread(&_verticesData[i].bone[0], boneidxsize, 1, fp);
			fread(&_verticesData[i].bone[1], boneidxsize, 1, fp);
			fread(&_verticesData[i].wait[0], sizeof(_verticesData[i].wait[0]), 1, fp);
		}
		else if (waitnum == 2) {
			fread(&_verticesData[i].bone[0], boneidxsize, 1, fp);
			fread(&_verticesData[i].bone[1], boneidxsize, 1, fp);
			fread(&_verticesData[i].bone[2], boneidxsize, 1, fp);
			fread(&_verticesData[i].bone[3], boneidxsize, 1, fp);
			fread(&_verticesData[i].wait[0], sizeof(float), 1, fp);
			fread(&_verticesData[i].wait[1], sizeof(float), 1, fp);
			fread(&_verticesData[i].wait[2], sizeof(float), 1, fp);
			fread(&_verticesData[i].wait[3], sizeof(float), 1, fp);
			
		}
		else if (waitnum == 3) {
			fread(&_verticesData[i].bone[0], boneidxsize, 1, fp);
			fread(&_verticesData[i].bone[1], boneidxsize, 1, fp);
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

	//ボーン
	unsigned int BoneNum = 0;
	fread(&BoneNum, sizeof(BoneNum), 1, fp);
	_boneData.resize(BoneNum);
	_bonename.resize(BoneNum);
	_bonenameE.resize(BoneNum);
	auto idx = 0;

	for (auto &b : _boneData) {

		//ボーン名
		unsigned int bytenum = 0;
		fread(&bytenum, sizeof(bytenum), 1, fp);

		std::wstring str;

		for (int i = 0; i < bytenum/2; ++i) {
			wchar_t c;
			fread(&c, sizeof(wchar_t), 1, fp);
			str += c;
		}
		_bonename[idx] = str;

		b.name = _bonename[idx];

		//ボーン英名
		fread(&bytenum, sizeof(bytenum), 1, fp);

		std::wstring strt;

		for (int i = 0; i < bytenum / 2; ++i) {
			wchar_t c;
			fread(&c, sizeof(wchar_t), 1, fp);
			strt += c;
		}
		_bonenameE[idx] = strt;

		//ポジション
		fread(&b.pos, sizeof(b.pos), 1, fp);

		//親ボーン
		fread(&b.parentbone, boneidxsize, 1, fp);

		//変形階層
		fread(&b.translevel, sizeof(b.translevel), 1, fp);

		//ボーンフラグ
		fread(&b.bitflag, sizeof(b.bitflag), 1, fp);

		//以下ボーンフラグを見て読み込み
		if (!(b.bitflag & 0x0001)) {
			fread(&b.offset, sizeof(b.offset), 1, fp);
		}
		else {
			fread(&b.toboneidx, boneidxsize, 1, fp);
		}
		if ((b.bitflag & 0x0200) || (b.bitflag & 0x0100)) {
			fread(&b.toparentidx, boneidxsize, 1, fp);
			fread(&b.grantrate, sizeof(b.grantrate), 1, fp);
		}
		if (b.bitflag & 0x0400) {
			fread(&b.axisvec, sizeof(b.axisvec), 1, fp);
		}
		if (b.bitflag & 0x0800) {
			fread(&b.axisXvec, sizeof(b.axisXvec), 1, fp);
			fread(&b.axisZvec, sizeof(b.axisZvec), 1, fp);
		}
		if (b.bitflag & 0x2000) {
			fread(&b.key, sizeof(b.key), 1, fp);
		}
		//IKの読み込み
		if (b.bitflag & 0x0020) {

			fread(&b.ik.targetboneidx, boneidxsize, 1, fp);
			fread(&b.ik.loop, sizeof(b.ik.loop), 1, fp);
			fread(&b.ik.loopangle, sizeof(b.ik.loopangle), 1, fp);
			fread(&b.ik.link, sizeof(b.ik.link), 1, fp);

			for (int i = 0; i < b.ik.link; ++i) {

				fread(&b.ik.linkboneidx, boneidxsize, 1, fp);
				fread(&b.ik.anglelimit, sizeof(b.ik.anglelimit), 1, fp);

				if (b.ik.anglelimit) {

					fread(&b.ik.bottomangle, sizeof(b.ik.bottomangle), 1, fp);
					fread(&b.ik.topangle, sizeof(b.ik.topangle), 1, fp);
				}
			}
		}
		_boneDataInfo[_bonename[idx]].first = idx;
		_boneDataInfo[_bonename[idx]].second = b;

		++idx;
	}

	unsigned int MorphNum = 0;
	fread(&MorphNum, sizeof(MorphNum), 1, fp);
	_morphData.resize(MorphNum);

	for (auto &m : _morphData) {

		//モーフ名
		unsigned int bytenum = 0;
		fread(&bytenum, sizeof(bytenum), 1, fp);

		for (int i = 0; i < bytenum / 2; ++i) {
			wchar_t c;
			fread(&c, sizeof(wchar_t), 1, fp);
		}

		//モーフ名英
		fread(&bytenum, sizeof(bytenum), 1, fp);

		for (int i = 0; i < bytenum / 2; ++i) {
			wchar_t c;
			fread(&c, sizeof(wchar_t), 1, fp);
		}

		fread(&m.panel, sizeof(m.panel), 1, fp);
		fread(&m.type, sizeof(m.type), 1, fp);
		fread(&m.offsetnum, sizeof(m.offsetnum), 1, fp);

		//グループ
		if (m.type == 0) {
			for (int i = 0; i < m.offsetnum; ++i) {
				fread(&m.groupMorph.vertidx, morphidxsize, 1, fp);
				fread(&m.groupMorph.morphnum, sizeof(m.groupMorph.morphnum), 1, fp);
			}
		}
		//頂点
		else if (m.type == 1) {
			for (int i = 0; i < m.offsetnum; ++i) {
				fread(&m.vertMorph.vertidx, vertidxsize, 1, fp);
				fread(&m.vertMorph.offset, sizeof(m.vertMorph.offset), 1, fp);
			}
		}
		//ボーン
		else if (m.type == 2) {
			for (int i = 0; i < m.offsetnum; ++i) {
				fread(&m.boneMorph.vertidx, boneidxsize, 1, fp);
				fread(&m.boneMorph.moveOffset, sizeof(m.boneMorph.moveOffset), 1, fp);
				fread(&m.boneMorph.rollOffset, sizeof(m.boneMorph.rollOffset), 1, fp);
			}
		}
		//マテリアル
		else if (m.type == 8) {
			for (int i = 0; i < m.offsetnum; ++i) {
				fread(&m.matMorph.vertidx, matidxsize, 1, fp);
				fread(&m.matMorph.offsetclass, sizeof(m.matMorph.offsetclass), 1, fp);
				fread(&m.matMorph.diffuse, sizeof(m.matMorph.diffuse), 1, fp);
				fread(&m.matMorph.speculer, sizeof(m.matMorph.speculer), 1, fp);
				fread(&m.matMorph.specPow, sizeof(m.matMorph.specPow), 1, fp);
				fread(&m.matMorph.Ambient, sizeof(m.matMorph.Ambient), 1, fp);
				fread(&m.matMorph.edgecolor, sizeof(m.matMorph.edgecolor), 1, fp);
				fread(&m.matMorph.edgesize, sizeof(m.matMorph.edgesize), 1, fp);
				fread(&m.matMorph.texPow, sizeof(m.matMorph.texPow), 1, fp);
				fread(&m.matMorph.sphtexPow, sizeof(m.matMorph.sphtexPow), 1, fp);
				fread(&m.matMorph.toontexPow, sizeof(m.matMorph.toontexPow), 1, fp);
			}
		}
		//UV
		else{
			for (int i = 0; i < m.offsetnum; ++i) {
				fread(&m.uvMorph.vertidx, vertidxsize, 1, fp);
				fread(&m.uvMorph.offset, sizeof(m.uvMorph.offset), 1, fp);
			}
		}
	}

	fclose(fp);

	_texturePaths.resize(_matData.size());
	_ToonBuff.resize(_matData.size());
	auto folder = GetTexPath(filepath);

	for (int i = 0; i < MatNum; ++i) {
		if (_matData[i].toonidx != 0xff) {
			if (_matData[i].toonflag == 1) {
				toonfilepath = GetToonTexpathFromIndex(_matData[i].toonidx, folder);
				InitToon(toonfilepath, _dev, i);
			}
			else
			{
				toonfilepath = GetOriginToonTexpathFromIndex(_matData[i].toonidx, folder);
				InitToon(toonfilepath, _dev, i);
			}
		}
		else {
			InitToon("", _dev, i);
		}
	}

	for (int i = 0; i < MatNum; ++i)
	{
		if (_matData[i].textureIndex != 0xff) {
			_texturePaths[i] = folder + GetTexAstPath(_texpath[_matData[i].textureIndex].c_str());
		}
	}
}

void PMXModel::CreatModelTex(ID3D12Device * _dev)
{
	auto& _texturepath = _texturePaths;
	auto& pMat = _matData;

	_TexBuff.resize(_matData.size());
	_TexBuffspa.resize(_matData.size());
	_TexBuffsph.resize(_matData.size());

	for (int i = 0; i < pMat.size(); i++) {

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
		if (texpointExt == "tga") {
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

void PMXModel::RotationBone(const std::string & boneName, const DirectX::XMFLOAT4 & q1, const DirectX::XMFLOAT4 & q2, float t)
{
	auto node = _boneMap[StringToWStirng(boneName)];
	auto vec = XMLoadFloat3(&node.startPos);
	auto quaternion = XMLoadFloat4(&q1);
	auto quaternion2 = XMLoadFloat4(&q2);

	//平行移動
	auto pararelMove = XMMatrixTranslationFromVector(XMVectorScale(vec, -1));
	auto rota = XMMatrixRotationQuaternion(XMQuaternionSlerp(quaternion, quaternion2, t));
	auto pararelMove2 = XMMatrixTranslationFromVector(vec);

	_boneMatrices[node.boneidx] = pararelMove * rota * pararelMove2;
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
		mappedColor->diffuse_color = pMat[idx].diffuse;
		
		//アンビエント
		mappedColor->ambient = pMat[idx].ambient;

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
	char ToonTexName[16];
	std::string toonpath = "toon/";
	sprintf_s(ToonTexName, "toon%02d.bmp", idx + 1);
	toonpath += ToonTexName;
	if (PathFileExists(toonpath.c_str())) {
		return toonpath;
	}
	else {
		return folderpath + ToonTexName;
	}
}

std::string PMXModel::GetOriginToonTexpathFromIndex(int idx, std::string folderpath)
{
	char ToonTexName[256];
	std::string toonpath = "toon/";
	sprintf_s(ToonTexName, _texpath[idx].c_str());
	toonpath += ToonTexName;
	if (PathFileExists(toonpath.c_str())) {
		return toonpath;
	}
	else {
		return folderpath + ToonTexName;
	}
}

void PMXModel::InitToon(std::string path, ID3D12Device * _dev, size_t idx) {

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
		texDesc.DepthOrArraySize = metadata.arraySize;			//リソースが2Dで配列でもないので１
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//何次元テクスチャか(TEXTURE2D)
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;				//NONE
		texDesc.Format = metadata.format;						//例によって
		texDesc.Width = metadata.width;							//テクスチャ幅
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

ID3D12DescriptorHeap*& PMXModel::GetMatHeap()
{
	return _matHeap;
}

ID3D12DescriptorHeap *& PMXModel::GetBoneHeap()
{
	return _boneHeap;
}

void PMXModel::InitModel(ID3D12Device * _dev)
{
	InitDescriptorHeapDSV(_dev);
	InitRootSignature(_dev);
	InitPipeline(_dev);
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
		str.c_str(),
		-1,
		nullptr,
		0
	);

	std::wstring wstr;
	wstr.resize(ssize - 1);

	ssize = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		ssize);
	//assert(ssize == wstr.length() - 1);
	return wstr;
}

std::string PMXModel::WStringToStirng(const std::wstring& wstr) {
	auto ssize = WideCharToMultiByte(
		CP_ACP,
		WC_NO_BEST_FIT_CHARS,
		wstr.data(),
		wstr.length(),
		nullptr,
		0,
		0,
		0
	);

	std::string str;
	str.resize(ssize);

	ssize = WideCharToMultiByte(
		CP_ACP,
		WC_NO_BEST_FIT_CHARS,
		wstr.data(),
		wstr.length(),
		&str[0],
		ssize,
		0,
		0
	);

	assert(ssize == str.length());
	return str;
}

void PMXModel::InitBone(ID3D12Device* _dev) {
	flame = 30;

	_boneMatrices.resize(_boneData.size());

	//単位行列を返す
	std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());

	//マップ情報を構築
	auto& mbones = _boneData;
	for (int idx = 0; idx < mbones.size(); ++idx) {
		auto& b = _boneData[idx];
		auto& boneNode = _boneMap[b.name];
		boneNode.boneidx = idx;
		if (b.toboneidx < _boneData.size()) {
			boneNode.startPos = b.pos;
		}
	}

	for (auto& b : _boneMap) {
		//次のボーンを見る
		if (mbones[b.second.boneidx].parentbone >= mbones.size())continue;
		auto parentName = mbones[mbones[b.second.boneidx].parentbone].name;
		//親のボーンを検索して自分をプッシュする
		_boneMap[parentName].children.push_back(&b.second);
	}

	//ボーンバッファ
	size_t size = sizeof(XMMATRIX) * _boneData.size();
	size = (size + 0xff)&~0xff;

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_boneBuff));

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask					= 0;
	descHeapDesc.NumDescriptors				= 1;
	descHeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_boneHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = _boneBuff->GetGPUVirtualAddress();
	desc.SizeInBytes = size;
	auto handle = _boneHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateConstantBufferView(&desc, handle);

	result = _boneBuff->Map(0, nullptr, (void**)&mappedBoneMat);
	std::copy(_boneMatrices.begin(), _boneMatrices.end(), mappedBoneMat);
}

void PMXModel::InitMotion(const char* filepath, ID3D12Device* _dev)
{
	FILE* f;
	fopen_s(&f, filepath, "rb");
	fseek(f, 50, SEEK_SET);//最初の５０は無駄データ

	unsigned int Vnum = 0;
	fread(&Vnum, sizeof(Vnum), 1, f);

	_motions.resize(Vnum);

	for (auto &_m : _motions) {
		fread(&_m.BoneName, sizeof(_m.BoneName), 1, f);
		fread(&_m.FlameNo, sizeof(_m.FlameNo), 1, f);
		fread(&_m.Location, sizeof(_m.Location), 1, f);
		fread(&_m.quaternion, sizeof(_m.quaternion), 1, f);
		fread(&_m.Interpolation, sizeof(_m.Interpolation), 1, f);
	}

	std::sort(_motions.begin(), _motions.end(), [](PMX_VMD_MOTION&a, PMX_VMD_MOTION&b) {return a.FlameNo < b.FlameNo; });

	for (auto &f : _motions) {
		auto& interpolation = f.Interpolation;
		auto ax = interpolation[48];
		auto ay = interpolation[52];
		auto bx = interpolation[56];
		auto by = interpolation[60];
		f.bz1 = DirectX::XMFLOAT2(static_cast<float>(ax) / 127.f, static_cast<float>(ay) / 127.f);
		f.bz2 = DirectX::XMFLOAT2(static_cast<float>(bx) / 127.f, static_cast<float>(by) / 127.f);
		_animation[f.BoneName].emplace_back(f);
	}
	fclose(f);
}

void PMXModel::RecursiveMatrixMultiply(BoneNodePMX & node, DirectX::XMMATRIX & inMat)
{
	_boneMatrices[node.boneidx] *= inMat;
	for (auto &bnode : node.children) {
		RecursiveMatrixMultiply(*bnode, _boneMatrices[node.boneidx]);
	}
}

float PMXModel::CreatBezier(float x, const DirectX::XMFLOAT2 & a, const DirectX::XMFLOAT2 & b, const unsigned int n)
{
	if (a.x == a.y && b.x == b.y)return x;	//この場合式は直線なのでｘをかえす

	float t = x;							//はじめはtはｘと同じでよい

	float k0 = 1 + 3 * a.x - 3 * b.x;		//係数０

	float k1 = 3 * b.x - 6 * a.x;			//係数１

	float k2 = 3 * a.x;						//係数２

	const float epsilon = 0.0005f;			//誤差の範囲

	for (int i = 0; i < n; i++) {

		float r = (1 - t);

		float ft = (t*t*t)*k0 + (t*t)*k1 + t * k2 - x;		//f(t)高さを求める

		if (ft <= epsilon && ft >= epsilon)break;			//誤差の範囲内なら打ち切る

		float fdt = 3 * (t*t) * k0 + 2 * t * k1 + 3 * k2;	//f'tのこと,ftの微分結果

		if (fdt == 0)break;									//0徐算の場合打ち切る

		t = t - ft / fdt;									//ニュートン法により答えを近づけていく
	}
	float r = (1 - t);
	//tが求まったのでyを計算していく
	return (3 * r * r * t * a.y) + (3 * r * t * t * b.y) + (t * t * t);
}

void PMXModel::InitShader()
{
	auto wsize = Application::GetInstance().GetWIndowSize();

	//モデル用シェーダ
	auto result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &vertexShader, nullptr);
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &pixelShader, nullptr);

	//ビューポート設定
	_viewport.TopLeftX = 0;
	_viewport.TopLeftY = 0;
	_viewport.Width = wsize.w;
	_viewport.Height = wsize.h;
	_viewport.MaxDepth = 1.0f;
	_viewport.MinDepth = 0.0f;

	//シザー(切り取り)矩形
	_scissorRect.left = 0;
	_scissorRect.top = 0;
	_scissorRect.right = wsize.w;
	_scissorRect.bottom = wsize.h;
}

void PMXModel::Draw(ID3D12Device* _dev, ID3D12GraphicsCommandList* _cmdList, std::shared_ptr<Camera> _camera, ID3D12DescriptorHeap* _rtv1stDescHeap)
{
	float clearColor[] = { 0,0,0.5f,1.0f };

	auto heapStart = _rtv1stDescHeap->GetCPUDescriptorHandleForHeapStart();

	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(1, &heapStart, false, &_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	//クリアレンダーターゲット
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	//深度バッファをクリア
	_cmdList->ClearDepthStencilView(_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	_cmdList->IASetVertexBuffers(0, 1, &_vbView);
	_cmdList->IASetIndexBuffer(&_idxbView);

	//CBVデスクリプタヒープ設定
	_cmdList->SetDescriptorHeaps(1, &_camera->GetrgstDescHeap());
	_cmdList->SetGraphicsRootDescriptorTable(0, _camera->GetrgstDescHeap()->GetGPUDescriptorHandleForHeapStart());

	//ボーンヒープセット
	_cmdList->SetDescriptorHeaps(1, &_boneHeap);
	_cmdList->SetGraphicsRootDescriptorTable(2, _boneHeap->GetGPUDescriptorHandleForHeapStart());

	//頂点セット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//モデル表示
	_cmdList->SetDescriptorHeaps(1, &_matHeap);

	unsigned int offset = 0;

	auto mathandle = _matHeap->GetGPUDescriptorHandleForHeapStart();

	auto incriment_size =
		_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;


	for (auto& m : _matData) {
		_cmdList->SetGraphicsRootDescriptorTable(1, mathandle);
		mathandle.ptr += incriment_size;
		_cmdList->DrawIndexedInstanced(m.face_vert_cnt, 1, offset, 0, 0);
		offset += m.face_vert_cnt;
	}
}

void PMXModel::InitModelVertices(ID3D12Device* _dev)
{
	/*auto vdata = _model->GetverticesData();
	auto idata = _model->GetindexData();*/

	auto vdata = _verticesData;
	auto idata = _indexData;

	//verticesバッファ作成
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vdata.size() * sizeof(vdata[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_vertexModelBuffer)
	);

	//マップ
	D3D12_RANGE range = { 0,0 };
	//PMDvertex* _vBufferptr = nullptr;
	PMXVertex* _vBufferptr = nullptr;

	_vertexModelBuffer->Map(0, &range, (void**)&_vBufferptr);
	std::copy(vdata.begin(), vdata.end(), _vBufferptr);
	_vertexModelBuffer->Unmap(0, nullptr);

	_vbView.BufferLocation = _vertexModelBuffer->GetGPUVirtualAddress();
	_vbView.SizeInBytes = vdata.size() * sizeof(vdata[0]);
	_vbView.StrideInBytes = sizeof(vdata[0]);

	//インデックスバッファ作成
	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(idata.size() * sizeof(idata[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexModelBuffer)
	);

	//マップ
	unsigned short* Buffptr = nullptr;
	result = _indexModelBuffer->Map(0, &range, (void**)&Buffptr);

	std::copy(idata.begin(), idata.end(), Buffptr);
	_indexModelBuffer->Unmap(0, nullptr);

	_idxbView.BufferLocation = _indexModelBuffer->GetGPUVirtualAddress();
	_idxbView.SizeInBytes = idata.size() * sizeof(unsigned short);
	_idxbView.Format = DXGI_FORMAT_R16_UINT;
}

void PMXModel::InitRootSignature(ID3D12Device * _dev)
{
	//サンプラ
	D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//エッジの色
	samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;//特別なフィルタを使用しない
	samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc[0].MinLOD = 0.0f;
	samplerDesc[0].MipLODBias = 0.0f;
	samplerDesc[0].ShaderRegister = 0;
	samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//どのくらいシェーダに見せるか
	samplerDesc[0].RegisterSpace = 0;
	samplerDesc[0].MaxAnisotropy = 0;
	samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	samplerDesc[1] = samplerDesc[0];
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].ShaderRegister = 1;

	ID3DBlob* signature = nullptr;//ID3D12Blob=メモリオブジェクト
	ID3DBlob* error = nullptr;

	D3D12_DESCRIPTOR_RANGE descTblRange[4] = {};
	D3D12_ROOT_PARAMETER rootParam[3] = {};

	//デスクリプタレンジの設定
	//座標変換定数バッファ
	descTblRange[0].BaseShaderRegister = 0;//レジスタ番号
	descTblRange[0].NumDescriptors = 1;
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//マテリアル用定数バッファ
	descTblRange[1].BaseShaderRegister = 1;//レジスタ番号
	descTblRange[1].NumDescriptors = 1;
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//テクスチャ用バッファ(SRV){基本、sph、spa、toon}
	descTblRange[2].BaseShaderRegister = 0;//レジスタ番号
	descTblRange[2].NumDescriptors = 4;
	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//ボーン用定数バッファ
	descTblRange[3].BaseShaderRegister = 2;//レジスタ番号
	descTblRange[3].NumDescriptors = 1;
	descTblRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//paramの設定
	//座標変換
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0];

	//マテリアル＋テクスチャ
	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];

	//ボーン用
	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[2].DescriptorTable.pDescriptorRanges = &descTblRange[3];

	//ルートシグネチャ
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pParameters = rootParam;
	rsd.pStaticSamplers = samplerDesc;
	rsd.NumParameters = 3;
	rsd.NumStaticSamplers = 2;

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

ID3D12RootSignature *& PMXModel::GetRootSignature()
{
	return _rootSignature;
}

void PMXModel::InitPipeline(ID3D12Device* _dev)
{
	D3D12_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },

		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		//追加UV
		{"ADDUV",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		{"ADDUV",1,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		{"ADDUV",2,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		{"ADDUV",3,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		//ウェイトタイプ
		{"WEIGHT_TYPE",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },

		//ボーンイデックス
		{"BONEINDEX",0,DXGI_FORMAT_R32G32B32A32_SINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		//ウェイト
		{"WEIGHT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};


	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = _rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = layout;
	gpsDesc.InputLayout.NumElements = _countof(layout);

	//シェーダ
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);

	//レンダーターゲット
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpsDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_RENDER_TARGET_BLEND_DESC renderBlDesc = {};
	renderBlDesc.BlendEnable = true;
	renderBlDesc.BlendOp = D3D12_BLEND_OP_ADD;
	renderBlDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	renderBlDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	renderBlDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	renderBlDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	renderBlDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	renderBlDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//αブレンド
	D3D12_BLEND_DESC BlendDesc = {};
	BlendDesc.AlphaToCoverageEnable = false;
	BlendDesc.IndependentBlendEnable = false;
	BlendDesc.RenderTarget[0] = renderBlDesc;

	//その他
	gpsDesc.BlendState = BlendDesc;
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	//gpsDesc.SampleMask			= 0xffffffff;
	gpsDesc.SampleMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pipeline));
}

ID3D12PipelineState *& PMXModel::GetPipeline()
{
	return _pipeline;
}

void PMXModel::InitDescriptorHeapDSV(ID3D12Device* _dev)
{
	auto &app = Application::GetInstance();

	//デスクリプターヒープ
	D3D12_DESCRIPTOR_HEAP_DESC _dsvDesc = {};
	_dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	_dsvDesc.NodeMask = 0;
	_dsvDesc.NumDescriptors = 1;
	_dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	auto result = _dev->CreateDescriptorHeap(&_dsvDesc, IID_PPV_ARGS(&_dsvHeap));

	//深度バッファ
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

	//クリアバリュー
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

	//深度バッファービュー
	D3D12_DEPTH_STENCIL_VIEW_DESC _dsvVDesc = {};
	_dsvVDesc.Format = DXGI_FORMAT_D32_FLOAT;
	_dsvVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	_dsvVDesc.Texture2D.MipSlice = 0;
	_dsvVDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView(_dsvBuff, &_dsvVDesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());
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

void PMXModel::Update()
{
	static auto lasttime = GetTickCount();

	std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());
	Duration(static_cast<float>(GetTickCount() - lasttime) / 33.33333f);
	std::copy(_boneMatrices.begin(), _boneMatrices.end(), mappedBoneMat);

	if (GetTickCount() - lasttime > flame++ * 33.33333f) {
		lasttime = GetTickCount();
	}
	
}

void PMXModel::Duration(float flame) 
{
	//初期化
	MotionUpdate(flame);
}

void PMXModel::MotionUpdate(int flameNo)
{
	for (auto &anim : _animation) {
		auto &keyflames = anim.second;
		auto flameIt = std::find_if(keyflames.rbegin(), keyflames.rend(),
			[flameNo](const PMX_VMD_MOTION& motion) {return motion.FlameNo <= flameNo; });
		if (flameIt == keyflames.rend())continue;
		auto nextIt = flameIt.base();
		if (nextIt == keyflames.end()) {
			RotationBone(anim.first.c_str(), flameIt->quaternion);
		}
		else {
			auto a = flameIt->FlameNo;
			auto b = nextIt->FlameNo;
			auto t = (static_cast<float>(flameNo) - a) / (b - a);
			//線形補間により腕の長さを調整している(しないと短くなる)
			t = CreatBezier(t, nextIt->bz1, nextIt->bz2);
			RotationBone(anim.first.c_str(), flameIt->quaternion, nextIt->quaternion, t);
		}
	}
	//ツリーをトラバース
	XMMATRIX rootmat = XMMatrixIdentity();
	RecursiveMatrixMultiply(_boneMap[L"センター"], rootmat);
}

