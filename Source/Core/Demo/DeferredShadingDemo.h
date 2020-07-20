#pragma once
#include"Renderer.h"
#include"Camera.h"
#include"Timer.h"
#include<memory>
#include "Resource/GPUResource.h"
#include "Resource/FrameResource.h"
#include "Structure/MeshPkg.h"
#include "Structure/EnvironmenMap.h"
#include "Structure/Terrain.h"
using namespace OpenLight;


class DeferredShadingFrameResource :public CPUFrameResource
{
public:
	DeferredShadingFrameResource(ID3D12Device5* device, ID3D12CommandAllocator* cmdListAlloc,
		GPUDescriptorHeapWrap* descriptorHeap)
		:CPUFrameResource(device, cmdListAlloc, (1 << 20))
	{
		cbTrans = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBTrans)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBTrans))));

		cbCamera = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBCamera)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBCamera))));

		cbMaterial = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBMaterial)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBMaterial))));

		cbPointLights = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBPointLights)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBPointLights))));

		cbSkyTrans = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBTrans)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBTrans))));

		cbTerrainTrans = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBTrans)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBTrans))));

		cbTerrainMaterial = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBMaterial)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBMaterial))));

		cbGrassInfo = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBGrassInfo)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBGrassInfo))));

		cbPerlinNoise = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBPerlinNoiseInfo)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBPerlinNoiseInfo))));

		cbDummyMotorMeshTrans = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBTrans)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBTrans))));
		cbDummyWindVolumeMeshTrans = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBTrans)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBTrans))));


		ThrowIfFailed(cbTrans->Map(0, nullptr, reinterpret_cast<void**>(&cbTransPtr)));
		ThrowIfFailed(cbCamera->Map(0, nullptr, reinterpret_cast<void**>(&cbCameraPtr)));
		ThrowIfFailed(cbMaterial->Map(0, nullptr, reinterpret_cast<void**>(&cbMaterialPtr)));
		ThrowIfFailed(cbPointLights->Map(0, nullptr, reinterpret_cast<void**>(&cbPointLightsPtr)));
		ThrowIfFailed(cbSkyTrans->Map(0, nullptr, reinterpret_cast<void**>(&cbSkyTransPtr)));
		ThrowIfFailed(cbTerrainTrans->Map(0, nullptr, reinterpret_cast<void**>(&cbTerrainTransPtr)));
		ThrowIfFailed(cbTerrainMaterial->Map(0, nullptr, reinterpret_cast<void**>(&cbTerrainMaterialPtr)));
		ThrowIfFailed(cbGrassInfo->Map(0, nullptr, reinterpret_cast<void**>(&cbGrassInfoPtr)));
		ThrowIfFailed(cbPerlinNoise->Map(0, nullptr, reinterpret_cast<void**>(&cbPerlinNoisePtr)));
		ThrowIfFailed(cbDummyMotorMeshTrans->Map(0, nullptr, reinterpret_cast<void**>(&cbDummyMotorMeshTransPtr)));
		ThrowIfFailed(cbDummyWindVolumeMeshTrans->Map(0, nullptr, reinterpret_cast<void**>(&cbDummyWindVolumeMeshTransPtr)));

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = cbSkyTrans->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = PADCB(sizeof(CBTrans));
		cbSkyTransIndex = descriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		descriptorHeap->AddCBV(&cbSkyTransIndex, cbSkyTrans, cbvDesc);
	}

	~DeferredShadingFrameResource()
	{
		ReleaseCom(cbTrans);
		ReleaseCom(cbCamera);
		ReleaseCom(cbMaterial);
		ReleaseCom(cbPointLights);
		ReleaseCom(cbSkyTrans);
		ReleaseCom(cbTerrainTrans);
		ReleaseCom(cbTerrainMaterial);
		ReleaseCom(cbGrassInfo);
		ReleaseCom(cbPerlinNoise);
		ReleaseCom(cbDummyMotorMeshTrans);
		ReleaseCom(cbDummyWindVolumeMeshTrans);
	}
	// CBTrans
	CBTrans* cbTransPtr = nullptr;
	ID3D12Resource* cbTrans = nullptr;

	// CBCamera
	CBCamera* cbCameraPtr = nullptr;
	ID3D12Resource* cbCamera = nullptr;

	// CBMaterial
	CBMaterial* cbMaterialPtr = nullptr;
	ID3D12Resource* cbMaterial = nullptr;

	// CBPointLight
	CBPointLights* cbPointLightsPtr = nullptr;
	ID3D12Resource* cbPointLights = nullptr;

	// SkyBox Trans
	DescriptorIndex	cbSkyTransIndex = {};
	CBTrans* cbSkyTransPtr = nullptr;
	ID3D12Resource* cbSkyTrans = nullptr;

	// Terrain Trans
	CBTrans* cbTerrainTransPtr = nullptr;
	ID3D12Resource* cbTerrainTrans = nullptr;

	// Terrain Material
	CBMaterial* cbTerrainMaterialPtr = nullptr;
	ID3D12Resource* cbTerrainMaterial = nullptr;

	// Grass Info
	CBGrassInfo* cbGrassInfoPtr = nullptr;
	ID3D12Resource* cbGrassInfo = nullptr;

	// PerliNoise Info
	CBPerlinNoiseInfo* cbPerlinNoisePtr = nullptr;
	ID3D12Resource* cbPerlinNoise = nullptr;

	CBTrans* cbDummyMotorMeshTransPtr = nullptr;
	ID3D12Resource* cbDummyMotorMeshTrans = nullptr;

	CBTrans* cbDummyWindVolumeMeshTransPtr = nullptr;
	ID3D12Resource* cbDummyWindVolumeMeshTrans = nullptr;

};


class DeferredShadingDemo :public Renderer
{

public:
	DeferredShadingDemo();
	virtual ~DeferredShadingDemo();
	virtual void Init(HWND hWnd) override;
	virtual void Render() override;
	virtual void Resize(uint32_t width, uint32_t height) override;

protected:
	void Update();
	void InitPostprocess();
	void InitSkyBox();
	void InitIBL();
	void FPS();
	void RenderSkyBox();
	void RenderTerrain();
	void RenderGrass();
	void RenderDummy();

	void createSkyBoxAndIBL(const std::string& envMapName);

	// 创建 Diffuse Irradiance Map
	void createDiffuseIrradianceMap(
		DescriptorIndex& envMapSRV,
		DescriptorIndex& irrMapUAV);

	// 创建 Specular Prefilter Environment Map
	void createSpecularPrefilterEnvMap(
		DescriptorIndex& envMapSRV,
		DescriptorIndex& prefilterMapUAV,
		int thetaRes,
		int phiRes,
		int sampleCount,
		float roughness);

	// 创建 Specular BSDF Map
	void createSpecularBSDFMap(
		DescriptorIndex& preBSDFMapUAV);

	// Construct GBuffer Root Signature
	ID3D12RootSignature*					mConstructGBufferRootSignature    = nullptr;
	// Construct GBuffer PSO
	ID3D12PipelineState*					mConstructGBufferPSO		        = nullptr;
	// DeferredLighting Root Signature
	ID3D12RootSignature* mDeferredLightingRootSignature = nullptr;
	// DeferredLighting PSO
	ID3D12PipelineState* mDeferredLightingPSO = nullptr;
	// Terrain GBuffer Root Signature
	ID3D12RootSignature* mConstructGBufferTerrainRootSignature = nullptr;
	// Terrain GBuffer PSO
	ID3D12PipelineState* mConstructGBufferTerrainPSO = nullptr;
	// 


	// 纹理采样器
	DescriptorIndex							mSamplerIndices   = {};
	DescriptorIndex							mSamPointIndex    = {};

	// PBR 渲染 物体
	std::vector<std::shared_ptr<MeshPkg>>	mPBRMeshs         = {};

	D3D12_VIEWPORT							mViewport         = {};
	D3D12_RECT								mScissorRect      = {};

	// 渲染天空盒
	SkyBox									mSkyBox           = {};


	// GBuffer 0 : diffuse color
	ID3D12Resource* mGBuffer0                                 = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mGBuffer0Handle             = {};
	// GBuffer 1 : normal 
	ID3D12Resource* mGBuffer1                                 = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mGBuffer1Handle             = {};


	// 后处理 流程
	// 后处理 SRV RTV
	DescriptorIndex						mPostprocessSRVIndex[3];
	DescriptorIndex						mPostprocessRTVIndex[3];
	ID3D12RootSignature* mPostprocessSignature = nullptr;
	ID3D12PipelineState* mPostprocessPSO = nullptr;
	ID3D12Resource1* mQuadVB = nullptr;
	ID3D12Resource1* mQuadIB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW			mQuadVBView;
	D3D12_INDEX_BUFFER_VIEW				mQuadIBView;
	ID3D12Resource1* mSceneColorBuffer[AppConfig::NumFrames];

	// Upload 堆
	OpenLight::GPUUploadHeapWrap* mUploadHeap = nullptr;
	OpenLight::GPUDescriptorHeapWrap* mDescriptorHeap = nullptr;
	// 漫游相机
	RoamCamera* mCamera = nullptr;
	// CPUFrameResource
	std::vector<std::unique_ptr<DeferredShadingFrameResource>>	mCPUFrameResource;
	// 时间器
	Timer								mTimer;
	// 点光源
	CBPointLights						mPointLights;
	int									mSelectedLightIndex = 0;

	// 环境光 IBL
	EnvironmentMap						mEnvironmentMap;
	bool								mUseIBL = true;

	// 地形
	std::shared_ptr<Terrain>			mTerrain = nullptr;
	std::shared_ptr<Grass>				mGrass = nullptr;
	bool								mCullGrass = false;


	float								mDeltaTime = 0.f;

	// Dummy Mesh
	std::shared_ptr<MeshPkg>			mMotorMesh = nullptr;
	std::shared_ptr<MeshPkg>			mWindVolumeMesh = nullptr;
	ID3D12RootSignature* mDummyRootSignature = nullptr;
	ID3D12PipelineState* mDummyPSO = nullptr;

};