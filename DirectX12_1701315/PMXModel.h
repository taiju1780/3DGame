#pragma once
#include<vector>
#include<map>
#include<memory>
#include<array>
#include<DirectXMath.h>
#include "Camera.h"
#include <dxgi1_6.h>
#include <d3d12.h>

//�w�b�_���
struct PMXHeader {
	char magic[4];
	float version;
	char bitesize;
	char data[8];
};

//�R�����g��
struct ModelInfo {
	int ModelNamesize;
	int ModelNameEsize;
	int Commentsize;
	int CommentEsize;
};

//���_���
struct PMXVertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	DirectX::XMFLOAT4 addUv[4];
	unsigned char waitclass;
	int bone[4];
	float wait[4];
	DirectX::XMFLOAT3 sdefvec[3];
	float edge;
};

#pragma pack(1)

//�}�e���A��
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

//IK
struct IKData {
	int targetboneidx;
	int loop;
	float loopangle;
	int link;
	int linkboneidx;
	char anglelimit;
	DirectX::XMFLOAT3 bottomangle;
	DirectX::XMFLOAT3 topangle;
};

//�{�[��
struct BoneInfo {
	std::wstring name;
	DirectX::XMFLOAT3 pos;
	unsigned int parentbone;
	unsigned int translevel;
	unsigned short bitflag;
	DirectX::XMFLOAT3 offset;
	unsigned int toboneidx;
	unsigned int toparentidx;
	float grantrate;
	DirectX::XMFLOAT3 axisvec;
	DirectX::XMFLOAT3 axisXvec;
	DirectX::XMFLOAT3 axisZvec;
	unsigned int key;
	IKData ik;
};

//���_���[�t
struct VertexMorph {
	int vertidx;
	DirectX::XMFLOAT3 offset;
};

//UV���[�t
struct UVMorph {
	int vertidx;
	DirectX::XMFLOAT4 offset;
};

//�{�[�����[�t
struct BoneMorph {
	int vertidx;
	DirectX::XMFLOAT3 moveOffset;
	DirectX::XMFLOAT4 rollOffset;
};

//�}�e���A�����[�t
struct MaterialMorph {
	int vertidx;
	char offsetclass;
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT3 speculer;
	float specPow;
	DirectX::XMFLOAT3 Ambient;
	DirectX::XMFLOAT4 edgecolor;
	float edgesize;
	DirectX::XMFLOAT4 texPow;
	DirectX::XMFLOAT4 sphtexPow;
	DirectX::XMFLOAT4 toontexPow;
};

//�O���[�v���[�t
struct GroupMorph {
	int vertidx;
	float morphnum;
};

//���[�t�f�[�^
struct MorphData {
	char panel;
	char type;
	int offsetnum;
	VertexMorph vertMorph;
	UVMorph uvMorph;
	BoneMorph boneMorph;
	MaterialMorph matMorph;
	GroupMorph groupMorph;
};

#pragma pack()

struct PMXColor {
	DirectX::XMFLOAT4 diffuse_color;	//dr,dg,db : �����F
	DirectX::XMFLOAT3 specular_color;	//sr,sg,sb : ����F
	DirectX::XMFLOAT3 ambient;			//mr,mg,mb : ���F(Ambient)
};

struct PMX_VMD_MOTION {					// 111 Bytes // ���[�V����
	char BoneName[15];					// �{�[����
	unsigned int FlameNo;				// �t���[���ԍ�(�Ǎ����͌��݂̃t���[���ʒu��0�Ƃ������Έʒu)
	DirectX::XMFLOAT3 Location;			// �ʒu
	DirectX::XMFLOAT4 quaternion;		// Quaternion // ��]
	unsigned char Interpolation[64];	// [4][4][4] // �⊮
	DirectX::XMFLOAT2 bz1;				//�x�W�F�W��1
	DirectX::XMFLOAT2 bz2;				//�x�W�F�W��2
};

struct BoneNodePMX {
	int boneidx;
	DirectX::XMFLOAT3 startPos;//�{�[���n�_
	//DirectX::XMFLOAT3 endPos;//�{�[���n�_
	std::vector<BoneNodePMX*> children;
};

class PMXModel
{
private:
	//���f���ǂݍ���
	void LoadModel(const char * filepath, ID3D12Device* _dev);

	//���_���
	std::vector<PMXVertex> _verticesData;

	//�C���f�b�N�X���
	std::vector<unsigned int> _indexData;

	//�e�N�X�`���p�p�X
	std::vector<std::string> _texpath;

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

	//�{�[����
	//�{�[���f�[�^
	std::vector<BoneInfo> _boneData;

	std::vector<std::wstring> _bonename;
	std::map<std::wstring, std::pair<int,BoneInfo>> _boneDataInfo;
	std::vector<std::wstring> _bonenameE;
	std::map<std::wstring, BoneNodePMX> _boneMap;//�T�����߂̃}�b�v

	//�O���{�ɓn������
	std::vector<DirectX::XMMATRIX> _boneMatrices;

	void RotationBone(const std::string& boneName, const DirectX::XMFLOAT4& puaternion, const DirectX::XMFLOAT4& puaternion2 = DirectX::XMFLOAT4(), float t = 0.0f);

	ID3D12Resource* _boneBuff;
	ID3D12DescriptorHeap* _boneHeap = nullptr;
	DirectX::XMMATRIX* mappedBoneMat;

	//���[�t�f�[�^
	std::vector<MorphData> _morphData;

	//�}�e���A���f�[�^
	std::vector<Material> _matData;

	std::vector<ID3D12Resource*> _matBuffs;
	PMXColor* mappedColor = nullptr;
	ID3D12DescriptorHeap* _matHeap = nullptr;

	void InitMaterial(ID3D12Device * _dev);

	//str->wstr
	std::wstring StringToWStirng(const std::string& str);

	std::string WStringToStirng(const std::wstring & str);

	//toon
	void InitToon(std::string path, ID3D12Device * _dev, size_t idx);

	std::string toonfilepath;

	ID3D12Resource* _gladTexBuff = nullptr;

	std::vector<ID3D12Resource*> _ToonBuff;

	std::array<char[100], 10> toonTexNames;

	std::string GetToonTexpathFromIndex(int idx, std::string folderpath);
	std::string GetOriginToonTexpathFromIndex(int idx, std::string folderpath);

	//���[�V����
	void MotionUpdate(int flameNo);

	std::vector<PMX_VMD_MOTION> _motions;
	std::map<std::string, std::vector<PMX_VMD_MOTION>> _animation;
	void RecursiveMatrixMultiply(BoneNodePMX& node, DirectX::XMMATRIX& inMat); //�ċN�֐�

	float CreatBezier(float x, const DirectX::XMFLOAT2 & a, const DirectX::XMFLOAT2 & b, const unsigned int n = 16);
	unsigned int flame;

	//��������//�V�F�[�_�֘A
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	//���[�g�V�O�l�`���[
	ID3D12RootSignature* _rootSignature = nullptr;
	void InitRootSignature(ID3D12Device* _dev);

	//�p�C�v���C��
	ID3D12PipelineState* _pipeline = nullptr;
	void InitPipeline(ID3D12Device* _dev);

	//�r���[�|�[�g�A�V�U�[
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	//���_���
	void InitModelVertices(ID3D12Device * _dev); 
	ID3D12Resource* _indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW _idxbView = {};

	ID3D12Resource* _vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};

	ID3D12Resource* _vertexModelBuffer = nullptr;
	ID3D12Resource* _indexModelBuffer = nullptr; 

	
public:
	PMXModel(const char * filepath, ID3D12Device* _dev);
	~PMXModel();
	void Update();
	void Duration(float flame);
	ID3D12DescriptorHeap*& GetMatHeap();
	ID3D12DescriptorHeap*& GetBoneHeap();
	std::vector<Material> GetMatData();

	D3D12_INDEX_BUFFER_VIEW GetidxbView();
	D3D12_VERTEX_BUFFER_VIEW GetvView();

	void InitModel(ID3D12Device* _dev);
	void InitBone(ID3D12Device* _dev);
	void InitMotion(const char * filepath, ID3D12Device * _dev);

	ID3D12RootSignature*& GetRootSignature();

	ID3D12PipelineState*& GetPipeline();

	void InitShader();

	void Draw(ID3D12Device* _dev, ID3D12GraphicsCommandList* _cmdList, std::shared_ptr<Camera> _camera, ID3D12DescriptorHeap* _rtv1stDescHeap);
};

