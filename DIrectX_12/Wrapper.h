#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>

class Camera;
class PMDModel;

//struct PMDHeader {
//	char magic[3]; //"pmd"
//	float version;
//	char model_name[20];
//	char comment[256];
//};
//
//#pragma pack(1)
//struct PMDvertex {
//	float pos[3];				// x, y, z // ���W 
//	float normal_vec[3];		// nx, ny, nz // �@���x�N�g�� 
//	float uv[2];				// u, v // UV���W // MMD�͒��_UV 
//	unsigned short bone_num[2]; // �{�[���ԍ�1�A�ԍ�2 // ���f���ό`(���_�ړ�)���ɉe��
//	unsigned char weight;
//	unsigned char edge;
//};
//#pragma pack()
//
//struct PMDMaterial
//{
//	float diffuse_color[3];		//rgb
//	float alpha;
//	float specular;
//	float specular_color[3];	//rgb
//	float mirror_color[3];		//rgb
//	unsigned char toon_index;	//toon.bmp
//	unsigned char edge_flag;	//�֊s�A�e
//	//�����܂łS�U�o�C�g
//	unsigned int face_vert_count;	//�ʐϓ_��
//	char texture_file_name[20];		//�e�N�X�`���t�@�C����
//};

class Wrapper
{
private:

	ID3D12Device* _dev = nullptr;

	IDXGIFactory6* factory = nullptr;

	//RTV�f�X�N���v�^�q�[�v�֌W
	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;
	std::vector<ID3D12Resource*> _backBuffers;

	///RTV�p�̃f�X�N���v�^�[�q�[�v������
	void InitDescriptorHeapRTV();

	//�X���b�v�`�F�C��
	///�X���b�v�`�F�C��������
	void InitSwapChain();
	IDXGISwapChain4* _swapchain = nullptr;

	//�R�}���h
	///�R�}���h������
	void InitCommand();
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	ID3D12CommandQueue* _cmdQue = nullptr;
	void ExecuteCmd();

	//�t�F���X
	///�t�F���X������
	void InitFence();
	UINT64 _fenceValue;//���̒l�őҋ@���邩���s���邩���f����
	ID3D12Fence* _fence = nullptr; 
	///Queue�����f���Ă��̊֐��Œl���X�V����
	void WaitExcute();

	//���_�f�[�^
	void InitVertices();
	ID3D12Resource* _vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};

	//�V�F�[�_�֘A
	void InitShader();
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	//���[�g�V�O�l�`���[
	void InitRootSignature();
	ID3D12RootSignature* _rootSignature = nullptr;
	
	//�p�C�v���C��
	void InitPipeline();
	ID3D12PipelineState* _pipeline = nullptr;

	//�r���[�|�[�g�A�V�U�[
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	//�l�p�`�`��p�̃C���f�b�N�X�֌W
	ID3D12Resource* _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _idxbView = {};

	//�摜�֌W
	void InitTexture();
	ID3D12Resource* _texbuff;
	
	ID3D12DescriptorHeap* _texrtvHeap = nullptr;
	ID3D12DescriptorHeap* _texsrvHeap = nullptr;

	//�J����
	std::shared_ptr<Camera> _camera;

	//model
	std::shared_ptr<PMDModel> _model;
	/*std::vector<PMDvertex> _verticesData;
	std::vector<unsigned short> _indexData;

	void InitModel(const char * filepath);*/

	//���_������
	void InitModelVertices();
	ID3D12Resource* _vertexModelBuffer = nullptr;
	ID3D12Resource* _indexModelBuffer = nullptr;

	//�[�x
	void InitDescriptorHeapDSV();
	ID3D12Resource* _dsvBuff;
	ID3D12DescriptorHeap* _dsvHeap = nullptr;
	ID3D12DescriptorHeap* _depthSrvHeap = nullptr;

public:
	Wrapper(HINSTANCE h, HWND hwnd);
	~Wrapper();
	void Update();
};

