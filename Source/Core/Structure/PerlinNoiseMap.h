#pragma once

#include"Utility.h"
#include<string>
#include<vector>
#include"ObjMeshLoader.h"
#include "Structure/CBStruct.h"
#include"Resource/GPUResource.h"
#include "Structure/AABB.h"
using namespace OpenLight;

class PerlinNoise2D
{	
public:
	PerlinNoise2D(ID3D12Device5* device,
		int width, int height,
		int xLatticeSize,
		int yLatticeSize,
		int iterations,
		XMFLOAT2 seeds,
		GPUDescriptorHeapWrap* descriptorHeap);
	~PerlinNoise2D()
	{
//		ReleaseCom(latticeVecsMap);
		ReleaseCom(perlinNoiseMap);
		ReleaseCom(mCreateNoiseMapSignature);
//		ReleaseCom(mCreateLatticeVecsPSO);
		ReleaseCom(mCreateNoiseMapPSO);
	}
	void createPerlinNoise(ID3D12GraphicsCommandList* cmdList,
		ID3D12Resource* cbPerlinNoiseGPU);
	
//	ID3D12Resource*			latticeVecsMap                    = nullptr;
	ID3D12Resource*			perlinNoiseMap                    = nullptr;
	DescriptorIndex			perlinNoiseSRVIndex               = {};

//	DescriptorIndex			latticeVecsAndPerlinNoiseUAVIndex = {};
	DescriptorIndex			perlinNoiseUAVIndex               = {};
	CBPerlinNoiseInfo		cbPerlinNoiseInfo;
	
protected:
	ID3D12Device5*			mDevice                           = nullptr;
	GPUDescriptorHeapWrap*  mDescriptorHeap                   = nullptr;
	int						mWidth                            = 0;
	int						mHeight                           = 0;
	int						mXLatticeSize                     = 0;
	int						mYLatticeSize                     = 0;
	int						mIterations                       = 0;
	
	ID3D12RootSignature*	mCreateNoiseMapSignature          = nullptr;
//	ID3D12PipelineState*	mCreateLatticeVecsPSO             = nullptr;
	ID3D12PipelineState*	mCreateNoiseMapPSO                = nullptr;

};
