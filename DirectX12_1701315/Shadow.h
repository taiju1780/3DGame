#pragma once
#include "d3dx12.h"

#pragma comment(lib,"d3d12.lib")

class Shadow
{
private:
	//シャドウマップ
	ID3D12DescriptorHeap * _shadowDsvHeap;
	ID3D12DescriptorHeap* _shadowSrvHeap;
	ID3D12Resource* _shadowbuff;

	ID3D12PipelineState* _shadowPipeline;
	ID3D12RootSignature* _shadowRootSignature;

	ID3DBlob* _shadowVshader;
	ID3DBlob* _shadowPshader;

	void InitShaders();

	size_t RoundupPowerOf2(size_t size);
	void CreateDSVSRV(ID3D12Device *_dev);

public:
	Shadow(ID3D12Device *_dev);
	~Shadow();

	ID3D12PipelineState*& GetShadowPipeline();
	ID3D12RootSignature*& GetShadowRootSignature();
	ID3D12DescriptorHeap *& GetDsvHeap();
	ID3D12DescriptorHeap *& GetSrvHeap();
	ID3D12Resource *& Getbuff();
	void InitRootSignature(ID3D12Device *_dev);
	void InitPipline(ID3D12Device *_dev);
};

