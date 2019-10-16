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

struct PMDbone {
	char bone_name[20];						// �{�[����
	unsigned short parent_bone_index;		// �e�{�[���ԍ�(�Ȃ��ꍇ��0xFFFF)
	unsigned short tail_pos_bone_index;		// tail�ʒu�̃{�[���ԍ�(�`�F�[�����[�̏ꍇ��0xFFFF 0 ���⑫2) // �e�F�q��1�F���Ȃ̂ŁA��Ɉʒu���ߗp
	unsigned char bone_type;				// �{�[���̎��
	unsigned short ik_parent_bone_index;	// IK�{�[���ԍ�(�e��IK�{�[���B�Ȃ��ꍇ��0)
	DirectX::XMFLOAT3 bone_head_pos;		// x, y, z // �{�[���̃w�b�h�̈ʒu
};

struct BoneNode {
	int boneidx;
	DirectX::XMFLOAT3 startPos;//�{�[���n�_
	//DirectX::XMFLOAT3 endPos;//�{�[���n�_
	std::vector<BoneNode*> children;
};

struct VMD_MOTION {						// 111 Bytes // ���[�V����
	char BoneName[15];					// �{�[����
	unsigned int FlameNo;				// �t���[���ԍ�(�Ǎ����͌��݂̃t���[���ʒu��0�Ƃ������Έʒu)
	float Location[3];					// �ʒu
	DirectX::XMFLOAT4 quaternion;		// Quaternion // ��]
	unsigned char Interpolation[64];	// [4][4][4] // �⊮
	DirectX::XMFLOAT2 bz1;				//�x�W�F�W��1
	DirectX::XMFLOAT2 bz2;				//�x�W�F�W��2
};

class PMDModel
{
private:
	//���f��
	std::vector<PMDvertex> _verticesData;
	std::vector<unsigned short> _indexData;
	std::vector<PMDMaterial> _matData;

	void InitModel(const char * filepath, ID3D12Device* _dev);
	void InitMaterial(ID3D12Device* _dev);

	//�}�e���A��
	std::vector<ID3D12Resource*> _matBuffs;
	PMDColor* mappedColor = nullptr;
	ID3D12DescriptorHeap* _matHeap = nullptr;

	std::wstring StringToWStirng(const std::string& str);

	//�e�N�X�`��
	std::vector<std::string> _texturePaths;
	void CreatModelTex(ID3D12Device* _dev);

	std::vector<ID3D12Resource*> _TexBuff;
	std::vector<ID3D12Resource*> _TexBuffspa;
	std::vector<ID3D12Resource*> _TexBuffsph;

	ID3D12Resource* _whiteTexbuff = nullptr;
	ID3D12Resource* _blackTexbuff = nullptr;

	void CreateWhiteTexture(ID3D12Device* _dev);
	void CreateBlackTexture(ID3D12Device* _dev);

	//born
	std::vector<PMDbone> _bones;
	std::vector<DirectX::XMMATRIX> _boneMatrices;//�{�[���s��]���p
	std::map<std::string, BoneNode> _boneMap;//�T�����߂̃}�b�v
	
	void RotationBone(const std::string& boneName, const DirectX::XMFLOAT4& puaternion, const DirectX::XMFLOAT4& puaternion2 = DirectX::XMFLOAT4(), float t = 0.0f);
	
	ID3D12Resource* _boneBuffer = nullptr;
	ID3D12DescriptorHeap* _boneHeap = nullptr;
	DirectX::XMMATRIX* mappedBoneMat;

	//���[�V����

	std::vector<VMD_MOTION> _motions;
	std::map<std::string, std::vector<VMD_MOTION>> _animation;
	void RecursiveMatrixMultiply(BoneNode& node, DirectX::XMMATRIX& inMat); //�ċN�֐�
	
	void MotionUpdate(int flameNo);
	float CreatBezier(float x, const DirectX::XMFLOAT2 & a, const DirectX::XMFLOAT2 & b, const unsigned int n = 16);
	unsigned int duration = 0;
	unsigned int flame;

	//Toon
	void InitToon(std::string path,ID3D12Device * _dev, size_t idx);

	std::string toonfilepath;

	ID3D12Resource* _gladTexBuff = nullptr;

	std::vector<ID3D12Resource*> _ToonBuff;

	std::array<char[100], 10> toonTexNames;

	std::string GetToonTexpathFromIndex(int idx, std::string folderpath);

	void CreateGraduation(ID3D12Device* _dev);

public:
	PMDModel(const char * filepath, ID3D12Device* _dev);
	~PMDModel();
	void Update();
	std::vector<PMDvertex> GetverticesData();
	std::vector<unsigned short> GetindexData();
	std::vector<PMDMaterial> GetmatData();
	ID3D12DescriptorHeap*& GetMatHeap();
	ID3D12DescriptorHeap*& GetBoneHeap();

	const unsigned int Duration();
	void InitMotion(const char* filepath, ID3D12Device* _dev);
	void InitBone(ID3D12Device* _dev);
};

