#pragma once
#include<vector>
#include<map>
#include<memory>
#include<array>
#include<DirectXMath.h>
#include "Camera.h"
#include <dxgi1_6.h>
#include <d3d12.h>

//ヘッダ情報
struct PMXHeader {
	char magic[4];
	float version;
	char bitesize;
	char data[8];
};

//コメント類
struct ModelInfo {
	int ModelNamesize;
	int ModelNameEsize;
	int Commentsize;
	int CommentEsize;
};

//頂点情報
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

//マテリアル
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

//ボーン
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

//頂点モーフ
struct VertexMorph {
	int vertidx;
	DirectX::XMFLOAT3 offset;
};

//UVモーフ
struct UVMorph {
	int vertidx;
	DirectX::XMFLOAT4 offset;
};

//ボーンモーフ
struct BoneMorph {
	int vertidx;
	DirectX::XMFLOAT3 moveOffset;
	DirectX::XMFLOAT4 rollOffset;
};

//マテリアルモーフ
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

//グループモーフ
struct GroupMorph {
	int vertidx;
	float morphnum;
};

//モーフデータ
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
	DirectX::XMFLOAT4 diffuse_color;	//dr,dg,db : 減衰色
	DirectX::XMFLOAT3 specular_color;	//sr,sg,sb : 光沢色
	DirectX::XMFLOAT3 ambient;			//mr,mg,mb : 環境色(Ambient)
};

struct PMX_VMD_MOTION {					// 111 Bytes // モーション
	char BoneName[15];					// ボーン名
	unsigned int FlameNo;				// フレーム番号(読込時は現在のフレーム位置を0とした相対位置)
	DirectX::XMFLOAT3 Location;			// 位置
	DirectX::XMFLOAT4 quaternion;		// Quaternion // 回転
	unsigned char Interpolation[64];	// [4][4][4] // 補完
	DirectX::XMFLOAT2 bz1;				//ベジェ係数1
	DirectX::XMFLOAT2 bz2;				//ベジェ係数2
};

struct BoneNodePMX {
	int boneidx;
	DirectX::XMFLOAT3 startPos;//ボーン始点
	//DirectX::XMFLOAT3 endPos;//ボーン始点
	std::vector<BoneNodePMX*> children;
};

class PMXModel
{
private:
	//モデル読み込み
	void LoadModel(const char * filepath, ID3D12Device* _dev);

	//頂点情報
	std::vector<PMXVertex> _verticesData;

	//インデックス情報
	std::vector<unsigned int> _indexData;

	//テクスチャ用パス
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

	//ボーン名
	//ボーンデータ
	std::vector<BoneInfo> _boneData;

	std::vector<std::wstring> _bonename;
	std::map<std::wstring, std::pair<int,BoneInfo>> _boneDataInfo;
	std::vector<std::wstring> _bonenameE;
	std::map<std::wstring, BoneNodePMX> _boneMap;//探すためのマップ

	//グラボに渡すため
	std::vector<DirectX::XMMATRIX> _boneMatrices;

	void RotationBone(const std::string& boneName, const DirectX::XMFLOAT4& puaternion, const DirectX::XMFLOAT4& puaternion2 = DirectX::XMFLOAT4(), float t = 0.0f);

	ID3D12Resource* _boneBuff;
	ID3D12DescriptorHeap* _boneHeap = nullptr;
	DirectX::XMMATRIX* mappedBoneMat;

	//モーフデータ
	std::vector<MorphData> _morphData;

	//マテリアルデータ
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

	//モーション
	void MotionUpdate(int flameNo);

	std::vector<PMX_VMD_MOTION> _motions;
	std::map<std::string, std::vector<PMX_VMD_MOTION>> _animation;
	void RecursiveMatrixMultiply(BoneNodePMX& node, DirectX::XMMATRIX& inMat); //再起関数

	float CreatBezier(float x, const DirectX::XMFLOAT2 & a, const DirectX::XMFLOAT2 & b, const unsigned int n = 16);
	unsigned int flame;

	//初期化類//シェーダ関連
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	//ルートシグネチャー
	ID3D12RootSignature* _rootSignature = nullptr;
	void InitRootSignature(ID3D12Device* _dev);

	//パイプライン
	ID3D12PipelineState* _pipeline = nullptr;
	void InitPipeline(ID3D12Device* _dev);

	//ビューポート、シザー
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	//頂点情報
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

