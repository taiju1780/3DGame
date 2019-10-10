#pragma once
#include<vector>
#include<map>
#include<array>
#include<DirectXMath.h>
#include <dxgi1_6.h>
#include <d3d12.h>

struct PMDHeader {
	char magic[3]; //"pmd"
	float version;
	char model_name[20];
	char comment[256];
};

#pragma pack(1)
struct PMDvertex {
	float pos[3];				// x, y, z // 座標 
	float normal_vec[3];		// nx, ny, nz // 法線ベクトル 
	float uv[2];				// u, v // UV座標 // MMDは頂点UV 
	unsigned short bone_num[2]; // ボーン番号1、番号2 // モデル変形(頂点移動)時に影響
	unsigned char weight;
	unsigned char edge;
};
#pragma pack()

struct PMDMaterial
{
	DirectX::XMFLOAT3 diffuse_color;		//rgb
	float alpha;
	float specular;
	DirectX::XMFLOAT3 specular_color;	//rgb
	DirectX::XMFLOAT3 mirror_color;		//rgb
	unsigned char toon_index;	//toon.bmp
	unsigned char edge_flag;	//輪郭、影
	//ここまで４６バイト
	unsigned int face_vert_count;	//面積点数
	char texture_file_name[20];		//テクスチャファイル名

};

struct PMDColor {
	DirectX::XMFLOAT4 diffuse_color;	//dr,dg,db : 減衰色
	DirectX::XMFLOAT4 specular_color;	//sr,sg,sb : 光沢色
	DirectX::XMFLOAT3 ambient;			//mr,mg,mb : 環境色(Ambient)
};

struct PMDbone {
	char bone_name[20];						// ボーン名
	unsigned short parent_bone_index;		// 親ボーン番号(ない場合は0xFFFF)
	unsigned short tail_pos_bone_index;		// tail位置のボーン番号(チェーン末端の場合は0xFFFF 0 →補足2) // 親：子は1：多なので、主に位置決め用
	unsigned char bone_type;				// ボーンの種類
	unsigned short ik_parent_bone_index;	// IKボーン番号(影響IKボーン。ない場合は0)
	DirectX::XMFLOAT3 bone_head_pos;		// x, y, z // ボーンのヘッドの位置
};

class PMDModel
{
private:
	//モデル
	std::vector<PMDvertex> _verticesData;
	std::vector<unsigned short> _indexData;
	std::vector<PMDMaterial> _matData;

	void InitModel(const char * filepath, ID3D12Device* _dev);
	void InitMaterial(ID3D12Device* _dev);

	//マテリアル
	std::vector<ID3D12Resource*> _matBuffs;
	PMDColor* mappedColor = nullptr;
	ID3D12DescriptorHeap* _matHeap;

	std::wstring StringToWStirng(const std::string& str);

	//テクスチャ
	std::vector<std::string> _texturePaths;
	void CreatModelTex(ID3D12Device* _dev);

	std::vector<ID3D12Resource*> _TexBuff;
	std::vector<ID3D12Resource*> _TexBuffspa;
	std::vector<ID3D12Resource*> _TexBuffsph;

	ID3D12Resource* _whiteTexbuff;
	ID3D12Resource* _blackTexbuff;

	void CreateWhiteTexture(ID3D12Device* _dev);
	void CreateBlackTexture(ID3D12Device* _dev);

	//born
	std::vector<PMDbone> _bones;

	//Toon
	void InitToon(std::string path,ID3D12Device * _dev, size_t idx);

	std::string toonfilepath;

	ID3D12Resource* _gladTexBuff;

	std::vector<ID3D12Resource*> _ToonBuff;

	std::array<char[100], 10> toonTexNames;

	std::string GetToonTexpathFromIndex(int idx, std::string folderpath);

	void CreateGraduation(ID3D12Device* _dev);

public:
	PMDModel(const char * filepath, ID3D12Device* _dev);
	~PMDModel();

	std::vector<PMDvertex> GetverticesData();
	std::vector<unsigned short> GetindexData();
	std::vector<PMDMaterial> GetmatData();
	ID3D12DescriptorHeap*& GetMatHeap();

};

