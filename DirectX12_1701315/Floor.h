#pragma once
#include "d3dx12.h"

#pragma comment(lib,"d3d12.lib")

class Floor
{
private:
	ID3DBlob * floorVertexShader = nullptr;
	ID3DBlob* floorPixelShader = nullptr;

	ID3D12RootSignature* _floorRootSignature;
	ID3D12PipelineState* _floorPipeline;

	//ビューポート
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;

	ID3D12Resource* _floorbuff;
	D3D12_VERTEX_BUFFER_VIEW _fvbView = {};


public:
	Floor(ID3D12Device* _dev);
	~Floor();

	void InitVerticesFloor(ID3D12Device* _dev);

	void InitShaders();

	void InitPiplineState(ID3D12Device* _dev);

	void InitRootSignature(ID3D12Device* _dev);

	ID3D12RootSignature*& GetRootSignature();
	ID3D12PipelineState*& _GetPipeline();
	D3D12_VERTEX_BUFFER_VIEW& GetView();
};

