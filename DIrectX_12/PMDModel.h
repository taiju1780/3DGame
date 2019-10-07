#pragma once
#include<vector>
#include<map>
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

class PMDModel
{
private:
	//モデル
	std::vector<PMDvertex> _verticesData;
	std::vector<unsigned short> _indexData;
	std::vector<PMDMaterial> _matData;

	void InitModel(const char * filepath);
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
	ID3D12Resource* _whiteTexbuff;

	void CreateWhiteTexture(ID3D12Device* _dev);

public:
	PMDModel(const char * filepath, ID3D12Device* _dev);
	~PMDModel();

	std::vector<PMDvertex> GetverticesData();
	std::vector<unsigned short> GetindexData();
	std::vector<PMDMaterial> GetmatData();
	ID3D12DescriptorHeap*& GetMatHeap();

};

