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
	float pos[3];				// x, y, z // ���W 
	float normal_vec[3];		// nx, ny, nz // �@���x�N�g�� 
	float uv[2];				// u, v // UV���W // MMD�͒��_UV 
	unsigned short bone_num[2]; // �{�[���ԍ�1�A�ԍ�2 // ���f���ό`(���_�ړ�)���ɉe��
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
	unsigned char edge_flag;	//�֊s�A�e
	//�����܂łS�U�o�C�g
	unsigned int face_vert_count;	//�ʐϓ_��
	char texture_file_name[20];		//�e�N�X�`���t�@�C����

};

struct PMDColor {
	DirectX::XMFLOAT4 diffuse_color;	//dr,dg,db : �����F
	DirectX::XMFLOAT4 specular_color;	//sr,sg,sb : ����F
	DirectX::XMFLOAT3 ambient;			//mr,mg,mb : ���F(Ambient)
};

class PMDModel
{
private:
	//���f��
	std::vector<PMDvertex> _verticesData;
	std::vector<unsigned short> _indexData;
	std::vector<PMDMaterial> _matData;

	void InitModel(const char * filepath);
	void InitMaterial(ID3D12Device* _dev);

	//�}�e���A��
	std::vector<ID3D12Resource*> _matBuffs;
	PMDColor* mappedColor = nullptr;
	ID3D12DescriptorHeap* _matHeap;

	std::wstring StringToWStirng(const std::string& str);

	//�e�N�X�`��
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

