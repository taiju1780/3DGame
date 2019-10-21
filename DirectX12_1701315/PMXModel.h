#pragma once
#include<vector>
#include<map>
#include<array>
#include<DirectXMath.h>
#include <dxgi1_6.h>
#include <d3d12.h>

struct PMXHeader {
	char magic[4];
	float version;
	char bitesize;
	char data[8];
};

struct ModelInfo {
	int ModelNamesize;
	int ModelNameEsize;
	int Commentsize;
	int CommentEsize;
};


#pragma pack(1)

struct PMXVertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	DirectX::XMFLOAT4 addUv[4];
	char waitclass;
	int bone[4];
	float wait[4];
	DirectX::XMFLOAT3 sdefvec[3];
	float edge;
};

struct Material {
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT3 specular;
	float specPower;
	DirectX::XMFLOAT3 ambient;
	unsigned char bitFlag;
	DirectX::XMFLOAT4 edgeColer;
	float edgesize;
	unsigned int textureIndex;
	unsigned int sphIndex;
	unsigned char sphmode;
	unsigned char toonflag;
	unsigned int toonidx;
	int face_vert_cnt;
};

#pragma pack()

struct PMXColor {
	DirectX::XMFLOAT4 diffuse_color;	//dr,dg,db : å∏êäêF
	DirectX::XMFLOAT3 specular_color;	//sr,sg,sb : åıëÚêF
	DirectX::XMFLOAT3 ambient;			//mr,mg,mb : ä¬ã´êF(Ambient)
};

class PMXModel
{
private:
	void LoadModel(const char * filepath, ID3D12Device* _dev);
	std::vector<PMXVertex> _verticesData;
	std::vector<unsigned short> _indexData;
	std::vector<std::string> _texpath;
	std::vector<Material> _matData;

	std::vector<ID3D12Resource*> _matBuffs;
	PMXColor* mappedColor = nullptr;
	ID3D12DescriptorHeap* _matHeap = nullptr;

	void CreateWhiteTexture(ID3D12Device* _dev);
	ID3D12Resource* _whiteTexbuff;
	void CreateBlackTexture(ID3D12Device* _dev);
	ID3D12Resource* _blackTexbuff;
	void CreateGraduation(ID3D12Device* _dev);
		
	void CreatModelTex(ID3D12Device * _dev); 
	std::vector<std::string> _texturePaths;
	std::vector<ID3D12Resource*> _TexBuff;
	std::vector<ID3D12Resource*> _TexBuffspa;
	std::vector<ID3D12Resource*> _TexBuffsph;

	void InitMaterial(ID3D12Device * _dev);

	std::wstring StringToWStirng(const std::string& str);

	//toon
	void InitToon(std::string path, ID3D12Device * _dev, size_t idx);

	std::string toonfilepath;

	ID3D12Resource* _gladTexBuff = nullptr;

	std::vector<ID3D12Resource*> _ToonBuff;

	std::array<char[100], 10> toonTexNames;

	std::string GetToonTexpathFromIndex(int idx, std::string folderpath);

public:
	PMXModel(const char * filepath, ID3D12Device* _dev);
	~PMXModel();
	std::vector<PMXVertex> GetverticesData();
	std::vector<unsigned short> GetindexData();
	std::vector<Material> GetmatData();
	ID3D12DescriptorHeap*& GetMatHeap();
};

