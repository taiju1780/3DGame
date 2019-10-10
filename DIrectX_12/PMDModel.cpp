#include "PMDModel.h"
#include <iostream>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <Windows.h>
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <vector>
#include<Shlwapi.h> 

//�����N
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

		//�e�N�X�`���f�X�N
		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Alignment			= 0;									//�擪����Ȃ̂�0
		texDesc.DepthOrArraySize	= metadata.arraySize;					//���\�[�X��2D�Ŕz��ł��Ȃ��̂łP
		texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//�������e�N�X�`����(TEXTURE2D)
		texDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;				//NONE
		texDesc.Format				= metadata.format;						//��ɂ����
		texDesc.Width				= metadata.width;						//�e�N�X�`����
		texDesc.Height				= metadata.height;						//�e�N�X�`������
		texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;			//����ł��Ȃ��̂�UNKNOWN
		texDesc.MipLevels			= metadata.mipLevels;					//�~�b�v�g���ĂȂ��̂�0
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

	//�e�N�X�`���f�X�N
	D3D12_RESOURCE_DESC WtexDesc = {};
	WtexDesc.Alignment			= 0;									//�擪����Ȃ̂�0
	WtexDesc.DepthOrArraySize	= 1;									//���\�[�X��2D�Ŕz��ł��Ȃ��̂łP
	WtexDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//�������e�N�X�`����(TEXTURE2D)
	WtexDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;			//NONE
	WtexDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;			//��ɂ����
	WtexDesc.Width				= 4;									//�e�N�X�`����
	WtexDesc.Height				= 4;									//�e�N�X�`������
	WtexDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;		//����ł��Ȃ��̂�UNKNOWN
	WtexDesc.MipLevels			= 1;									//�~�b�v�g���ĂȂ��̂�0
	WtexDesc.SampleDesc.Count	= 1;
	WtexDesc.SampleDesc.Quality = 0;

	auto result = _dev->CreateCommittedResource(
		&Wheapprop,
		D3D12_HEAP_FLAG_NONE,
		&WtexDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_whiteTexbuff));

	//���e�N�X�`��
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

	//�e�N�X�`���f�X�N
	D3D12_RESOURCE_DESC WtexDesc = {};
	WtexDesc.Alignment = 0;									//�擪����Ȃ̂�0
	WtexDesc.DepthOrArraySize = 1;									//���\�[�X��2D�Ŕz��ł��Ȃ��̂łP
	WtexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//�������e�N�X�`����(TEXTURE2D)
	WtexDesc.Flags = D3D12_RESOURCE_FLAG_NONE;			//NONE
	WtexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			//��ɂ����
	WtexDesc.Width = 4;									//�e�N�X�`����
	WtexDesc.Height = 4;									//�e�N�X�`������
	WtexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;		//����ł��Ȃ��̂�UNKNOWN
	WtexDesc.MipLevels = 1;									//�~�b�v�g���ĂȂ��̂�0
	WtexDesc.SampleDesc.Count = 1;
	WtexDesc.SampleDesc.Quality = 0;

	auto result = _dev->CreateCommittedResource(
		&Wheapprop,
		D3D12_HEAP_FLAG_NONE,
		&WtexDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_blackTexbuff));

	//���e�N�X�`��
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
		//00000000111111112222222233333333�A�݂����Ȋ����̂�����
		std::fill_n(itr, 4, Color(brightness, brightness, brightness, 0xff));
		--brightness;
	}

	result = _gladTexBuff->WriteToSubresource
	(0, nullptr, cdata.data(), 4 * sizeof(Color), cdata.size() * sizeof(Color));
}

void PMDModel::InitToon(std::string path, ID3D12Device * _dev, size_t idx) {
	
	_ToonBuff.resize(_matData.size());


	TexMetadata metadata = {};
	ScratchImage scratchimg = {};

	_ToonBuff[idx] = _gladTexBuff;

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

	//�e�N�X�`���f�X�N
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Alignment			= 0;									//�擪����Ȃ̂�0
	texDesc.DepthOrArraySize	= metadata.arraySize;					//���\�[�X��2D�Ŕz��ł��Ȃ��̂łP
	texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//�������e�N�X�`����(TEXTURE2D)
	texDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;				//NONE
	texDesc.Format				= metadata.format;						//��ɂ����
	texDesc.Width				= metadata.width;						//�e�N�X�`����
	texDesc.Height				= metadata.height;						//�e�N�X�`������
	texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;			//����ł��Ȃ��̂�UNKNOWN
	texDesc.MipLevels			= metadata.mipLevels;					//�~�b�v�g���ĂȂ��̂�0
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

PMDModel::PMDModel(const char * filepath, ID3D12Device* _dev)
{
	InitModel(filepath, _dev);
	CreateWhiteTexture(_dev);
	CreateBlackTexture(_dev);
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

void PMDModel::InitModel(const char * filepath, ID3D12Device* _dev)
{
	FILE *fp;

	fopen_s(&fp, filepath, "rb");

	//�w�b�_�ǂݍ���
	PMDHeader pmdheader = {};
	fread(&pmdheader.magic, sizeof(pmdheader.magic), 1, fp);
	fread(&pmdheader.version, sizeof(pmdheader) - sizeof(pmdheader.magic) - 1, 1, fp);

	//���_
	unsigned int Vnum = 0;
	fread(&Vnum, sizeof(Vnum), 1, fp);

	_verticesData.resize(Vnum);

	for (auto i = 0; i < Vnum; ++i) {
		fread(&_verticesData[i], sizeof(PMDvertex), 1, fp);
	}

	//�C���f�b�N�X
	unsigned int IdxNum = 0;
	fread(&IdxNum, sizeof(IdxNum), 1, fp);

	_indexData.resize(IdxNum);
	for (auto i = 0; i < IdxNum; ++i) {
		fread(&_indexData[i], sizeof(unsigned short), 1, fp);
	}

	//�}�e���A��
	unsigned int matNum = 0;
	fread(&matNum, sizeof(matNum), 1, fp);

	_matData.resize(matNum);
	for (auto &mat : _matData) {
		fread(&mat, MatSizeFlont, 1, fp);
		fread(&mat.face_vert_count, MatSizeBack, 1, fp);
	}

	//�{�[�� 
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

	//�\�� 
	unsigned short skinNum = 0; fread(&skinNum, sizeof(skinNum), 1, fp);
	for (int i = 0; i < skinNum; ++i) {
		fseek(fp, 20, SEEK_CUR);
		unsigned int vertNum = 0;
		fread(&vertNum, sizeof(vertNum), 1, fp);
		fseek(fp, 1, SEEK_CUR); fseek(fp, 16 * vertNum, SEEK_CUR);
	}

	//�\���p�\�� 
	unsigned char skinDispNum = 0;
	fread(&skinDispNum, sizeof(skinDispNum), 1, fp);
	fseek(fp, skinDispNum * sizeof(unsigned short), SEEK_CUR);

	//�\���p�{�[���� 
	unsigned char boneDispNum = 0;
	fread(&boneDispNum, sizeof(boneDispNum), 1, fp);
	fseek(fp, 50 * boneDispNum, SEEK_CUR);

	//�\���{�[�����X�g 
	unsigned int dispBoneNum = 0;
	fread(&dispBoneNum, sizeof(dispBoneNum), 1, fp);

	fseek(fp, 3 * dispBoneNum, SEEK_CUR);			//�p�� //�p���Ή��t���O 
	unsigned char englishFlg = 0;
	fread(&englishFlg, sizeof(englishFlg), 1, fp);

	if (englishFlg) {								//���f����20�o�C�g+256�o�C�g�R�����g
		fseek(fp, 20 + 256, SEEK_CUR);				//�{�[����20�o�C�g*�{�[����
		fseek(fp, boneNum * 20, SEEK_CUR);			//(�\�-1)*20�o�C�g�B-1�Ȃ̂̓x�[�X�����Ԃ�
		fseek(fp, (skinNum - 1) * 20, SEEK_CUR);		//�{�[����*50�o�C�g�B
		fseek(fp, boneDispNum * 50, SEEK_CUR);
	}

	//�g�D�[��
	fread(toonTexNames.data(), sizeof(char) * 100, toonTexNames.size(), fp);

	fclose(fp);

	size_t idx = 0;
	_texturePaths.resize(matNum);
	auto folder = GetTexPath(filepath);

	for (auto &mat : _matData) {
		if (mat.toon_index != 0xff) {
			toonfilepath = GetToonTexpathFromIndex(mat.toon_index, folder);
			InitToon(toonfilepath, _dev, idx);
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

		//�f�B�t���[�Y
		mappedColor->diffuse_color.x = pMat[idx].diffuse_color.x;
		mappedColor->diffuse_color.y = pMat[idx].diffuse_color.y;
		mappedColor->diffuse_color.z = pMat[idx].diffuse_color.z;
		mappedColor->diffuse_color.w = pMat[idx].alpha;

		//�A���r�G���g
		mappedColor->ambient.x = pMat[idx].mirror_color.x;
		mappedColor->ambient.y = pMat[idx].mirror_color.y;
		mappedColor->ambient.z = pMat[idx].mirror_color.z;

		//�X�y�L�����[
		mappedColor->specular_color.x = pMat[idx].specular_color.x;
		mappedColor->specular_color.y = pMat[idx].specular_color.y;
		mappedColor->specular_color.z = pMat[idx].specular_color.z;
		mappedColor->specular_color.w = pMat[idx].specular;

		m->Unmap(0, nullptr);

		idx++;
	}

	//�}�e���A���̃q�[�v
	D3D12_DESCRIPTOR_HEAP_DESC matHeapDesc = {};
	matHeapDesc.Flags					= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matHeapDesc.NodeMask				= 0;
	matHeapDesc.NumDescriptors			= pMat.size() * 5;
	matHeapDesc.Type					= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&matHeapDesc, IID_PPV_ARGS(&_matHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC matViewDesc = {};
	auto matH = _matHeap->GetCPUDescriptorHandleForHeapStart();

	for (auto i = 0; i < pMat.size(); ++i) {
		//�萔�o�b�t�@
		matViewDesc.BufferLocation	= _matBuffs[i]->GetGPUVirtualAddress();
		matViewDesc.SizeInBytes		= size;
		_dev->CreateConstantBufferView(&matViewDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
		//�e�N�X�`���̃q�[�v
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension			= D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels		= 1;

		//t1
		srvDesc.Format = _TexBuff[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(_TexBuff[i], &srvDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//t2�̃V�F�[�_���\�[�X�r���[(sph)
		srvDesc.Format = _TexBuffsph[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(_TexBuffsph[i], &srvDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//t3�̃V�F�[�_���\�[�X�r���[(spa)
		srvDesc.Format = _TexBuffspa[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(_TexBuffspa[i], &srvDesc, matH);
		matH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
		//t4�̃V�F�[�_���\�[�X�r���[(toon)
		srvDesc.Format = _ToonBuff[i]->GetDesc().Format;
		_dev->CreateShaderResourceView(_ToonBuff[i], &srvDesc, matH);
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
