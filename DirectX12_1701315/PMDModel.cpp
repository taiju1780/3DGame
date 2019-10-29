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

	std::string GetExtension(const std::string& modelpath) {
		int idx = modelpath.rfind('.');
		return modelpath.substr(idx + 1, modelpath.length() - idx - 1);
	}

	std::pair<std::string, std::string>
		SplitFileName(const std::string& path,const char split = '*'){
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

PMDModel::PMDModel(const char * filepath, ID3D12Device* _dev)
{
	CreateGraduation(_dev);
	InitModel(filepath, _dev);
	CreateWhiteTexture(_dev);
	CreateBlackTexture(_dev);
	CreatModelTex(_dev);
	InitMaterial(_dev);
}

void PMDModel::CreatModelTex(ID3D12Device * _dev)
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
		heapprop.Type					= D3D12_HEAP_TYPE_CUSTOM;
		heapprop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		heapprop.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
		heapprop.CreationNodeMask		= 0;
		heapprop.VisibleNodeMask		= 0;

		//テクスチャデスク
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Alignment			= 0;									//先頭からなので0
		texDesc.DepthOrArraySize	= metadata.arraySize;					//リソースが2Dで配列でもないので１
		texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//何次元テクスチャか(TEXTURE2D)
		texDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;				//NONE
		texDesc.Format				= metadata.format;						//例によって
		texDesc.Width				= metadata.width;						//テクスチャ幅
		texDesc.Height				= metadata.height;						//テクスチャ高さ
		texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;			//決定できないのでUNKNOWN
		texDesc.MipLevels			= metadata.mipLevels;					//ミップ使ってないので0
		texDesc.SampleDesc.Count	= 1;
		texDesc.SampleDesc.Quality	= 0;

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

void PMDModel::CreateWhiteTexture(ID3D12Device* _dev)
{
	D3D12_HEAP_PROPERTIES Wheapprop = {};
	Wheapprop.Type					= D3D12_HEAP_TYPE_CUSTOM;
	Wheapprop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	Wheapprop.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
	Wheapprop.CreationNodeMask		= 1;
	Wheapprop.VisibleNodeMask		= 1;

	//テクスチャデスク
	D3D12_RESOURCE_DESC WtexDesc = {};
	WtexDesc.Alignment			= 0;									//先頭からなので0
	WtexDesc.DepthOrArraySize	= 1;									//リソースが2Dで配列でもないので１
	WtexDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//何次元テクスチャか(TEXTURE2D)
	WtexDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;			//NONE
	WtexDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;			//例によって
	WtexDesc.Width				= 4;									//テクスチャ幅
	WtexDesc.Height				= 4;									//テクスチャ高さ
	WtexDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;		//決定できないのでUNKNOWN
	WtexDesc.MipLevels			= 1;									//ミップ使ってないので0
	WtexDesc.SampleDesc.Count	= 1;
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

void PMDModel::CreateBlackTexture(ID3D12Device* _dev)
{
	D3D12_HEAP_PROPERTIES Wheapprop = {};
	Wheapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	Wheapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	Wheapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	Wheapprop.CreationNodeMask = 1;
	Wheapprop.VisibleNodeMask = 1;

	//テクスチャデスク
	D3D12_RESOURCE_DESC WtexDesc = {};
	WtexDesc.Alignment			= 0;										//先頭からなので0
	WtexDesc.DepthOrArraySize	= 1;										//リソースが2Dで配列でもないので１
	WtexDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//何次元テクスチャか(TEXTURE2D)
	WtexDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;					//NONE
	WtexDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;				//例によって
	WtexDesc.Width				= 4;										//テクスチャ幅
	WtexDesc.Height				= 4;										//テクスチャ高さ
	WtexDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;				//決定できないのでUNKNOWN
	WtexDesc.MipLevels			= 1;										//ミップ使ってないので0
	WtexDesc.SampleDesc.Count	= 1;
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

std::string PMDModel::GetToonTexpathFromIndex(int idx, std::string folderpath) {
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

void PMDModel::CreateGraduation(ID3D12Device* _dev)
{
	size_t size = sizeof(float) * 256;
	//size = (size + 0xff)&~0xff;

	CD3DX12_HEAP_PROPERTIES hProp = {};
	hProp.Type					= D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
	hProp.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	hProp.CreationNodeMask		= 1;
	hProp.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
	hProp.VisibleNodeMask		= 1;

	CD3DX12_RESOURCE_DESC rDesc = {};
	rDesc.Height				= 256;
	rDesc.Width					= 4;
	rDesc.SampleDesc.Count		= 1;
	rDesc.Dimension				= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rDesc.Flags					= D3D12_RESOURCE_FLAG_NONE;
	rDesc.Format				= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	rDesc.DepthOrArraySize		= 1;
	rDesc.Layout				= D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rDesc.MipLevels				= 1;

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

void PMDModel::InitMotion(const char* filepath, ID3D12Device* _dev)
{
	FILE* f;
	fopen_s(&f, filepath, "rb");
	fseek(f, 50,SEEK_SET);//最初の５０は無駄データ

	unsigned int Vnum = 0;
	fread(&Vnum, sizeof(Vnum),1, f);

	_motions.resize(Vnum);

	for (auto &_m : _motions) {
		fread(&_m.BoneName, sizeof(_m.BoneName), 1, f);
		fread(&_m.FlameNo, sizeof(_m.FlameNo), 1, f);
		fread(&_m.Location, sizeof(_m.Location), 1, f);
		fread(&_m.quaternion, sizeof(_m.quaternion), 1, f);
		fread(&_m.Interpolation, sizeof(_m.Interpolation), 1, f);
	}

	std::sort(_motions.begin(), _motions.end(), [](VMD_MOTION&a, VMD_MOTION&b) {return a.FlameNo < b.FlameNo; });

	for (auto &f : _motions) {
		auto& interpolation = f.Interpolation;
		auto ax = interpolation[48];
		auto ay = interpolation[52];
		auto bx = interpolation[56];
		auto by = interpolation[60];
		f.bz1 = DirectX::XMFLOAT2(static_cast<float>(ax) / 127.f, static_cast<float>(ay) / 127.f);
		f.bz2 = DirectX::XMFLOAT2(static_cast<float>(bx) / 127.f, static_cast<float>(by) / 127.f);
		_animation[f.BoneName].emplace_back(f);
		duration = max(f.FlameNo, duration);
	}
	fclose(f);
}

float PMDModel::CreatBezier(float x, const DirectX::XMFLOAT2 & a, const DirectX::XMFLOAT2 & b, const unsigned int n)
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

void PMDModel::RecursiveMatrixMultiply(BoneNode & node, DirectX::XMMATRIX & inMat)
{
	_boneMatrices[node.boneidx] *= inMat;
	for (auto &bnode : node.children) {
		RecursiveMatrixMultiply(*bnode, _boneMatrices[node.boneidx]);
	}
}

void PMDModel::MotionUpdate(int flameNo)
{
	for (auto &anim : _animation) {
		auto &keyflames = anim.second;
		auto flameIt = std::find_if(keyflames.rbegin(), keyflames.rend(), 
			[flameNo](const VMD_MOTION& motion) {return motion.FlameNo <= flameNo; });
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
	RecursiveMatrixMultiply(_boneMap["センター"], rootmat);
}

void PMDModel::InitToon(std::string path, ID3D12Device * _dev, size_t idx) {
	
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
		heapprop.Type					= D3D12_HEAP_TYPE_CUSTOM;
		heapprop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		heapprop.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
		heapprop.CreationNodeMask		= 0;
		heapprop.VisibleNodeMask		= 0;

		//テクスチャデスク
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Alignment			= 0;									//先頭からなので0
		texDesc.DepthOrArraySize	= metadata.arraySize;					//リソースが2Dで配列でもないので１
		texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//何次元テクスチャか(TEXTURE2D)
		texDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;				//NONE
		texDesc.Format				= metadata.format;						//例によって
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

		_ToonBuff[idx] = tmpbuff;

		scratchimg.Release();
	}
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

ID3D12DescriptorHeap *& PMDModel::GetBoneHeap()
{
	return _boneHeap;
}

const unsigned int PMDModel::Duration()
{
	return duration + 30;
}

void PMDModel::InitModel(const char * filepath, ID3D12Device* _dev)
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

	//ボーン 
	unsigned short boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);
	_bones.resize(boneNum);

	for (auto& bone : _bones) {
		fread(&bone.bone_name, 24, 1, fp);
		fread(&bone.bone_type, sizeof(bone.bone_type), 1, fp);
		fread(&bone.ik_parent_bone_index, sizeof(bone.ik_parent_bone_index), 1, fp);
		fread(&bone.bone_head_pos, 12, 1, fp);
	}

	unsigned short ikNum = 0;
	fread(&ikNum, sizeof(ikNum), 1, fp);

	for (int i = 0; i < ikNum; ++i) {
		fseek(fp, 4, SEEK_CUR);
		unsigned char ikchainNum;
		fread(&ikchainNum, sizeof(ikchainNum), 1, fp);
		fseek(fp, 6, SEEK_CUR);
		fseek(fp, ikchainNum * sizeof(unsigned short), SEEK_CUR);
	}

	//表情 
	unsigned short skinNum = 0; fread(&skinNum, sizeof(skinNum), 1, fp);
	for (int i = 0; i < skinNum; ++i) {
		fseek(fp, 20, SEEK_CUR);
		unsigned int vertNum = 0;
		fread(&vertNum, sizeof(vertNum), 1, fp);
		fseek(fp, 1, SEEK_CUR); fseek(fp, 16 * vertNum, SEEK_CUR);
	}

	//表示用表情 
	unsigned char skinDispNum = 0;
	fread(&skinDispNum, sizeof(skinDispNum), 1, fp);
	fseek(fp, skinDispNum * sizeof(unsigned short), SEEK_CUR);

	//表示用ボーン名 
	unsigned char boneDispNum = 0;
	fread(&boneDispNum, sizeof(boneDispNum), 1, fp);
	fseek(fp, 50 * boneDispNum, SEEK_CUR);

	//表示ボーンリスト 
	unsigned int dispBoneNum = 0;
	fread(&dispBoneNum, sizeof(dispBoneNum), 1, fp);

	fseek(fp, 3 * dispBoneNum, SEEK_CUR);			//英名 //英名対応フラグ 
	unsigned char englishFlg = 0;
	fread(&englishFlg, sizeof(englishFlg), 1, fp);

	if (englishFlg) {								//モデル名20バイト+256バイトコメント
		fseek(fp, 20 + 256, SEEK_CUR);				//ボーン名20バイト*ボーン数
		fseek(fp, boneNum * 20, SEEK_CUR);			//(表情数-1)*20バイト。-1なのはベース部分ぶん
		fseek(fp, (skinNum - 1) * 20, SEEK_CUR);	//ボーン数*50バイト。
		fseek(fp, boneDispNum * 50, SEEK_CUR);
	}

	//トゥーン
	fread(toonTexNames.data(), sizeof(char) * 100, toonTexNames.size(), fp);

	fclose(fp);

	size_t idx = 0;
	_texturePaths.resize(matNum);

	_ToonBuff.resize(_matData.size());

	auto folder = GetTexPath(filepath);

	for (auto &mat : _matData) {
		if (mat.toon_index != 0xff) {
			toonfilepath = GetToonTexpathFromIndex(mat.toon_index, folder);
			InitToon(toonfilepath, _dev, idx);
		}
		else {
			InitToon("", _dev, idx);
		}
		if (std::strlen(mat.texture_file_name) > 0) {
			_texturePaths[idx] = folder + GetTexAstPath(mat.texture_file_name);
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
	matHeapDesc.NumDescriptors			= pMat.size() * 5;
	matHeapDesc.Type					= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&matHeapDesc, IID_PPV_ARGS(&_matHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC matViewDesc = {};
	auto matH = _matHeap->GetCPUDescriptorHandleForHeapStart();

	for (auto i = 0; i < pMat.size(); ++i) {
		//定数バッファ
		matViewDesc.BufferLocation	= _matBuffs[i]->GetGPUVirtualAddress();
		matViewDesc.SizeInBytes		= size;
		_dev->CreateConstantBufferView(&matViewDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
		//テクスチャのヒープ
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension			= D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels		= 1;

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

void PMDModel::InitBone(ID3D12Device* _dev) {
	flame = 30;
	_boneMatrices.resize(_bones.size());

	//単位行列を返す
	std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());

	//マップ情報を構築
	auto& mbones = _bones;
	for (int idx = 0; idx < mbones.size(); ++idx) {
		auto& b				= _bones[idx];
		auto& boneNode		= _boneMap[b.bone_name];
		boneNode.boneidx	= idx;
		boneNode.startPos	= b.bone_head_pos;
		//boneNode.endPos		= mbones[b.tail_pos_bone_index].bone_head_pos;
	}

	for (auto& b : _boneMap) {
		//次のボーンを見る
		if (mbones[b.second.boneidx].parent_bone_index >= mbones.size())continue;
		auto parentName = mbones[mbones[b.second.boneidx].parent_bone_index].bone_name;
		//親のボーンを検索して自分をプッシュする
		_boneMap[parentName].children.push_back(&b.second);
	}

	//ボーンバッファ
	size_t size = sizeof(XMMATRIX) * _bones.size();
	size = (size + 0xff)&~0xff;

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_boneBuffer));

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask					= 0;
	descHeapDesc.NumDescriptors				= 1;
	descHeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_boneHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation		= _boneBuffer->GetGPUVirtualAddress();
	desc.SizeInBytes		= size;
	auto handle				= _boneHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateConstantBufferView(&desc, handle);

	result = _boneBuffer->Map(0, nullptr, (void**)&mappedBoneMat);
	std::copy(_boneMatrices.begin(), _boneMatrices.end(), mappedBoneMat);
}

void PMDModel::RotationBone(const std::string & boneName, const DirectX::XMFLOAT4 & q1, const DirectX::XMFLOAT4 & q2, float t)
{
	auto node = _boneMap[boneName];
	auto vec = XMLoadFloat3(&node.startPos);
	auto quaternion = XMLoadFloat4(&q1);
	auto quaternion2 = XMLoadFloat4(&q2);

	//平行移動
	auto pararelMove = XMMatrixTranslationFromVector(XMVectorScale(vec, -1));
	auto rota = XMMatrixRotationQuaternion(XMQuaternionSlerp(quaternion, quaternion2, t));
	auto pararelMove2 = XMMatrixTranslationFromVector(vec);

	_boneMatrices[node.boneidx] = pararelMove * rota * pararelMove2;
}

void PMDModel::Update() {
	flame++;
	flame %= Duration() * 2;

	//初期化
	std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());
	MotionUpdate(flame / 2);
	std::copy(_boneMatrices.begin(), _boneMatrices.end(), mappedBoneMat);
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
