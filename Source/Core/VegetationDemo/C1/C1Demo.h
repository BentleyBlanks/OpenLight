#pragma once
#include"Renderer.h"
#include"Camera.h"
#include"Timer.h"
#include<memory>
#include "Resource/GPUResource.h"
#include "Resource/FrameResource.h"
#include "Structure/MeshPkg.h"
#include "Structure/EnvironmenMap.h"

using namespace OpenLight;


class VegetationC1FrameResource :public CPUFrameResource
{
public:
	VegetationC1FrameResource(ID3D12Device5* device,ID3D12CommandAllocator* cmdListAlloc,
		GPUDescriptorHeapWrap* descriptorHeap)
		:CPUFrameResource(device,cmdListAlloc,(1<<20))
	{
		cbTrans       = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBTrans)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBTrans))));

		cbCamera      = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBCamera)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBCamera))));

		cbMaterial    = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBMaterial)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBMaterial))));

		cbPointLights = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBPointLights)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBPointLights))));

		cbSkyTrans = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBTrans)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBTrans))));

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};

		cbvDesc.BufferLocation                  = cbTrans->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes                     = PADCB(sizeof(CBTrans));
		cbTransIndex                            = descriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		descriptorHeap->AddCBV(&cbTransIndex, cbTrans, cbvDesc);
		ThrowIfFailed(cbTrans->Map(0, nullptr, reinterpret_cast<void**>(&cbTransPtr)));

		cbCameraMaterialLightIndex              = descriptorHeap->Allocate(3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		cbvDesc.BufferLocation                  = cbCamera->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes                     = PADCB(sizeof(CBCamera));
		descriptorHeap->AddCBV(&cbCameraMaterialLightIndex, cbCamera, cbvDesc);
		ThrowIfFailed(cbCamera->Map(0, nullptr, reinterpret_cast<void**>(&cbCameraPtr)));

		cbvDesc.BufferLocation                  = cbMaterial->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes                     = PADCB(sizeof(CBMaterial));
		descriptorHeap->AddCBV(&cbCameraMaterialLightIndex, cbMaterial, cbvDesc);
		ThrowIfFailed(cbMaterial->Map(0, nullptr, reinterpret_cast<void**>(&cbMaterialPtr)));

		cbvDesc.BufferLocation                  = cbPointLights->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes                     = PADCB(sizeof(CBPointLights));
		descriptorHeap->AddCBV(&cbCameraMaterialLightIndex, cbPointLights, cbvDesc);
		ThrowIfFailed(cbPointLights->Map(0, nullptr, reinterpret_cast<void**>(&cbPointLightsPtr)));

		cbvDesc.BufferLocation                  = cbSkyTrans->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes                     = PADCB(sizeof(CBTrans));
		cbSkyTransIndex                         = descriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		descriptorHeap->AddCBV(&cbSkyTransIndex, cbSkyTrans, cbvDesc);
		ThrowIfFailed(cbSkyTrans->Map(0, nullptr, reinterpret_cast<void**>(&cbSkyTransPtr)));


	}

	~VegetationC1FrameResource()
	{
		ReleaseCom(cbTrans);
		ReleaseCom(cbCamera);
		ReleaseCom(cbMaterial);
		ReleaseCom(cbPointLights);
		ReleaseCom(cbSkyTrans);
	}
	// CBTrans
	DescriptorIndex			cbTransIndex                      = {};
	CBTrans*				cbTransPtr                        = nullptr;
	ID3D12Resource*			cbTrans							  = nullptr;

	// Camera Material PointLight Index
	DescriptorIndex			cbCameraMaterialLightIndex        = {};

	// CBCamera
	CBCamera*				cbCameraPtr                       = nullptr;
	ID3D12Resource*			cbCamera                          = nullptr;

	// CBMaterial
	CBMaterial*				cbMaterialPtr                     = nullptr;
	ID3D12Resource*			cbMaterial                        = nullptr;

	// CBPointLight
	CBPointLights*			cbPointLightsPtr                  = nullptr;
	ID3D12Resource*			cbPointLights                     = nullptr;

	// SkyBox Trans
	DescriptorIndex			cbSkyTransIndex                   = {};
	CBTrans*				cbSkyTransPtr                     = nullptr;
	ID3D12Resource*			cbSkyTrans                        = nullptr;

};


class VegetationC1Demo :public Renderer
{

public:
	VegetationC1Demo();
	virtual ~VegetationC1Demo();
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


	// 正常 PBR 渲染流程

	// 纹理采样器
	DescriptorIndex							mSamplerIndices   = {};
	// PBR 渲染 PSO
	ID3D12PipelineState*					mPSORGBA32        = nullptr;
	// PBR IBL 渲染 PSO
	ID3D12PipelineState*					mPSOIBLRGBA32	  = nullptr;
	// PBR 渲染 根签名
	ID3D12RootSignature*					mRootSignature    = nullptr;
	// PBR IBL 渲染 PSO
	ID3D12RootSignature*					mIBLRootSignature = nullptr;
	// PBR 渲染 物体
	std::vector<std::shared_ptr<MeshPkg>>	mPBRMeshs         = {};
	
	D3D12_VIEWPORT							mViewport         = {};
	D3D12_RECT								mScissorRect      = {};

	// 渲染天空盒
	SkyBox									mSkyBox           = {};
	

	// 创建 IBL 流程
	// 预计算 Diffuse Irradiance Map
	ID3D12PipelineState*					mPrecomputeDiffuseIrradiancePSO = nullptr;
	// Specular 部分
	ID3D12PipelineState*					mSpecularPrefilterEnvMapPSO     = nullptr;
	ID3D12PipelineState*					mSpecularPrecomputeBSDFMapPSO   = nullptr;

	// 后处理 流程

	// 后处理 SRV RTV
	DescriptorIndex						mPostprocessSRVIndex[3];
	DescriptorIndex						mPostprocessRTVIndex[3];
	ID3D12RootSignature*				mPostprocessSignature = nullptr;
	ID3D12PipelineState*				mPostprocessPSO       = nullptr;
	ID3D12Resource1*					mQuadVB               = nullptr;
	ID3D12Resource1*					mQuadIB               = nullptr;
	D3D12_VERTEX_BUFFER_VIEW			mQuadVBView;
	D3D12_INDEX_BUFFER_VIEW				mQuadIBView;
	ID3D12Resource1*					mSceneColorBuffer[AppConfig::NumFrames];

	// Upload 堆
	OpenLight::GPUUploadHeapWrap*		mUploadHeap = nullptr;
	OpenLight::GPUDescriptorHeapWrap*	mDescriptorHeap = nullptr;
	// 漫游相机
	RoamCamera*							mCamera = nullptr;
	// CPUFrameResource
	std::vector<std::unique_ptr<VegetationC1FrameResource>>	mCPUFrameResource;
	// 时间器
	Timer								mTimer;
	// 点光源
	CBPointLights						mPointLights;
	int									mSelectedLightIndex = 0;
	
	// 环境光 IBL
	EnvironmentMap						mEnvironmentMap;
	bool								mUseIBL = true;

};


