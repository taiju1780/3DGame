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



class PMXModel
{
private:
	void LoadModel(const char * filepath, ID3D12Device* _dev);
	std::vector<PMXVertex> _verticesData;
	std::vector<unsigned short> _indexData;
	std::vector<std::string> _texpath;
	std::vector<Material> _MatData;

public:
	PMXModel(const char * filepath, ID3D12Device* _dev);
	~PMXModel();
};

