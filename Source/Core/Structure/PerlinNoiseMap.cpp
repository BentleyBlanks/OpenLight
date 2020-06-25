#include "PerlinNoiseMap.h"
#include <d3dcompiler.h>
PerlinNoise2D::PerlinNoise2D(ID3D12Device5* device, 
	int width, int height, 
	int xLatticeSize, int yLatticeSize, 
	int iterations,
	XMFLOAT2 seeds,
	GPUDescriptorHeapWrap* descriptorHeap)
{
	mDevice         = device;
	mDescriptorHeap = descriptorHeap;
	mWidth          = width;
	mHeight         = height;
	mXLatticeSize   = xLatticeSize;
	mYLatticeSize   = yLatticeSize;
	mIterations     = iterations;

	cbPerlinNoiseInfo.seeds					   = seeds;
	cbPerlinNoiseInfo.iterations[0]            = mIterations;
	cbPerlinNoiseInfo.iterations[1]			   = mIterations;
	cbPerlinNoiseInfo.noiseMapAndLatticeRes[0] = width;
	cbPerlinNoiseInfo.noiseMapAndLatticeRes[1] = height;
	cbPerlinNoiseInfo.noiseMapAndLatticeRes[2] = xLatticeSize;
	cbPerlinNoiseInfo.noiseMapAndLatticeRes[3] = yLatticeSize;

	assert(width % xLatticeSize == 0 && height % yLatticeSize == 0);
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R32G32_FLOAT,
			mWidth, mHeight,
			1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&perlinNoiseMap)));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format                          = DXGI_FORMAT_R32G32_FLOAT;
	srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels             = 1;
	srvDesc.Texture2D.MostDetailedMip       = 0;
	srvDesc.Texture2D.PlaneSlice            = 0;
	srvDesc.Texture2D.ResourceMinLODClamp   = 0.f;
	perlinNoiseSRVIndex                     = descriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorHeap->AddSRV(&perlinNoiseSRVIndex, perlinNoiseMap, srvDesc);

	//xLatticeSize = (xLatticeSize >> (mIterations - 1));
	//yLatticeSize = (yLatticeSize >> (mIterations - 1));
	//assert(mWidth % xLatticeSize == 0);
	//assert(mHeight % yLatticeSize == 0);
	//int xLatticeRes = mWidth / xLatticeSize;
	//int yLatticeRes = mHeight / yLatticeSize;
	//
	//ThrowIfFailed(mDevice->CreateCommittedResource(
	//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	//	D3D12_HEAP_FLAG_NONE,
	//	&CD3DX12_RESOURCE_DESC::Tex2D(
	//		DXGI_FORMAT_R32G32_FLOAT,
	//		xLatticeRes, yLatticeRes,
	//		1, 1, 1, 0,
	//		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
	//	D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
	//	nullptr,
	//	IID_PPV_ARGS(&latticeVecsMap)));

	perlinNoiseUAVIndex = descriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format                           = DXGI_FORMAT_R32G32_FLOAT;
	uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice               = 0;
	uavDesc.Texture2D.PlaneSlice             = 0;
	descriptorHeap->AddUAV(&perlinNoiseUAVIndex, perlinNoiseMap, uavDesc);
	

	// 创建 Signature 和 PSO
	CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
//	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0);
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	CD3DX12_ROOT_PARAMETER1 rootParams[2];
	rootParams[0].InitAsConstantBufferView(0,0,D3D12_ROOT_DESCRIPTOR_FLAG_NONE,D3D12_SHADER_VISIBILITY_ALL);
	rootParams[1].InitAsDescriptorTable(1, ranges, D3D12_SHADER_VISIBILITY_ALL);
	
	// 根签名描述
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(2, rootParams,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	// 创建根签名
	WRL::ComPtr<ID3DBlob> pSignatureBlob;
	WRL::ComPtr<ID3DBlob> pErrorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_1,
		&pSignatureBlob,
		&pErrorBlob));
	ThrowIfFailed(mDevice->CreateRootSignature(0,
		pSignatureBlob->GetBufferPointer(),
		pSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&mCreateNoiseMapSignature)));

#if defined(_DEBUG)		
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif 
	ID3DBlob* cs = nullptr;
	//ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\PerlinNoiseCS.hlsl",
	//	nullptr, D3D_HLSL_DEFUALT_INCLUDE, "ComputeLatticeVectorsMainCS", "cs_5_0", compileFlags, 0, &cs, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\PerlinNoiseCS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "ComputePerlinNoiseMainCS", "cs_5_0", compileFlags, 0, &cs, nullptr));
	D3D12_COMPUTE_PIPELINE_STATE_DESC cpsDesc = {};
	cpsDesc.CachedPSO          = {};
	cpsDesc.CS.pShaderBytecode = cs->GetBufferPointer();
	cpsDesc.CS.BytecodeLength  = cs->GetBufferSize();
	cpsDesc.NodeMask           = 0;
	cpsDesc.Flags              = D3D12_PIPELINE_STATE_FLAG_NONE;
	cpsDesc.pRootSignature     = mCreateNoiseMapSignature;
	ThrowIfFailed(mDevice->CreateComputePipelineState(&cpsDesc, IID_PPV_ARGS(&mCreateNoiseMapPSO)));
	ReleaseCom(cs);

}

void PerlinNoise2D::createPerlinNoise(ID3D12GraphicsCommandList* cmdList,
	ID3D12Resource* cbPerlinNoiseGPU)
{
	// 将 PerlinNoise Map 状态从 PIXEL_SHAER_RESOURCE 切换至 UNOREDERED_ACCESS 
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = perlinNoiseMap;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(1, &barrier);

	cmdList->SetComputeRootSignature(mCreateNoiseMapSignature);
	cmdList->SetPipelineState(mCreateNoiseMapPSO);	
	
	ID3D12DescriptorHeap* heaps[] = {
		mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
	};
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->SetComputeRootConstantBufferView(0, cbPerlinNoiseGPU->GetGPUVirtualAddress());
	cmdList->SetComputeRootDescriptorTable(1, mDescriptorHeap->GPUHandle(perlinNoiseUAVIndex));

	//auto xLatticeSize = mXLatticeSize;
	//auto yLatticeSize = mYLatticeSize;
	//xLatticeSize = (xLatticeSize >> (mIterations - 1 )); 
	//yLatticeSize = (yLatticeSize >> (mIterations - 1 )); 
	//int xLatticeRes = mWidth / xLatticeSize;
	//int yLatticeRes = mHeight / yLatticeSize;
	//cmdList->Dispatch(xLatticeRes / 8 , yLatticeRes / 8, 1);

	// 创建 PerlinNoise Map

	cmdList->Dispatch(mWidth / 16, mHeight / 16, 1);

	// 把 PerlinNoise 的状态从 UAV 切换至
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = perlinNoiseMap;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(1, &barrier);

}
