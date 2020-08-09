#pragma once
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
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphResource.h"
#include "RenderGraph/RenderGraphCompiler.h"
using namespace OpenLight;


class DeferredShadingRenderGraphFrameResource :public CPUFrameResource
{
public:
	DeferredShadingRenderGraphFrameResource(ID3D12Device5* device, ID3D12CommandAllocator* cmdListAlloc,
		GPUDescriptorHeapWrap* descriptorHeap)
		:CPUFrameResource(device, cmdListAlloc, (1 << 20))
	{
		cbTrans = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBTrans)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBTrans))));

		cbCamera = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBCamera)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBCamera))));

		cbTerrainTrans = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBTrans)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBTrans))));

		cbTerrainMaterial = CPUResourceUploadHeap->createResource(device,
			PADCB(sizeof(CBMaterial)),
			CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBMaterial))));



		ThrowIfFailed(cbTrans->Map(0, nullptr, reinterpret_cast<void**>(&cbTransPtr)));
		ThrowIfFailed(cbCamera->Map(0, nullptr, reinterpret_cast<void**>(&cbCameraPtr)));

		ThrowIfFailed(cbTerrainTrans->Map(0, nullptr, reinterpret_cast<void**>(&cbTerrainTransPtr)));
		ThrowIfFailed(cbTerrainMaterial->Map(0, nullptr, reinterpret_cast<void**>(&cbTerrainMaterialPtr)));

	}

	~DeferredShadingRenderGraphFrameResource()
	{
		ReleaseCom(cbTrans);
		ReleaseCom(cbCamera);
		ReleaseCom(cbTerrainTrans);
		ReleaseCom(cbTerrainMaterial);
	}
	// CBTrans
	CBTrans* cbTransPtr = nullptr;
	ID3D12Resource* cbTrans = nullptr;

	// CBCamera
	CBCamera* cbCameraPtr = nullptr;
	ID3D12Resource* cbCamera = nullptr;



	// Terrain Trans
	CBTrans* cbTerrainTransPtr = nullptr;
	ID3D12Resource* cbTerrainTrans = nullptr;

	// Terrain Material
	CBMaterial* cbTerrainMaterialPtr = nullptr;
	ID3D12Resource* cbTerrainMaterial = nullptr;


};


class DeferredShadingRenderGraphDemo :public Renderer
{

public:
	DeferredShadingRenderGraphDemo();
	virtual ~DeferredShadingRenderGraphDemo();
	virtual void Init(HWND hWnd) override;
	virtual void Render() override;
	virtual void Resize(uint32_t width, uint32_t height) override;

protected:
	void Update();
	void InitPostprocess();
	void InitSkyBox();
	void FPS();
	void RenderSkyBox();

	void DepthPass(const RenderGraphPass* pass);
	void GBuffer(const RenderGraphPass* pass);
	void GBufferTerrain(const RenderGraphPass* pass);
	void Lighting(const RenderGraphPass* pass);
	void Postprocess(const RenderGraphPass* pass);

	void InitRenderGraph();

	// Construct GBuffer Root Signature
	ID3D12RootSignature* mConstructGBufferRootSignature = nullptr;
	// Construct GBuffer PSO
	ID3D12PipelineState* mConstructGBufferPSO = nullptr;
	// DeferredLighting Root Signature
	ID3D12RootSignature* mDeferredLightingRootSignature = nullptr;
	// DeferredLighting PSO
	ID3D12PipelineState* mDeferredLightingPSO = nullptr;
	// Terrain GBuffer Root Signature
	ID3D12RootSignature* mConstructGBufferTerrainRootSignature = nullptr;
	// Terrain GBuffer PSO
	ID3D12PipelineState* mConstructGBufferTerrainPSO = nullptr;
	// Depth Pass Root Signature
	ID3D12RootSignature* mDepthPassRootSignature = nullptr;
	// Depth Pass PSO
	ID3D12PipelineState* mDepthPassPSO = nullptr;


	// Render Graph
	RenderPassID		mDepthPassID;
	RenderPassID		mGBufferPassID;
	RenderPassID		mLightingPassID;
	RenderPassID		mPostprocessPassID;
	LogicalResourceID	mDepthBufferID;
	LogicalResourceID	mGBuffer0ID;
	LogicalResourceID   mGBuffer1ID;
	LogicalResourceID	mSceneBufferID;
	GraphCompiledResult mGraphCompiledResult;



	// ���������
	DescriptorIndex							mSamplerIndices = {};
	DescriptorIndex							mSamPointIndex = {};

	// PBR ��Ⱦ ����
	std::vector<std::shared_ptr<MeshPkg>>	mPBRMeshs = {};
	ID3D12Resource* m39DiffuseMap = nullptr;
	DescriptorIndex							m39DescriptorIndex = {};

	D3D12_VIEWPORT							mViewport = {};
	D3D12_RECT								mScissorRect = {};

	// ��Ⱦ��պ�
	//	SkyBox									mSkyBox = {};


	// ���� ����
	ID3D12RootSignature* mPostprocessSignature = nullptr;
	ID3D12PipelineState* mPostprocessPSO = nullptr;
	ID3D12Resource1* mQuadVB = nullptr;
	ID3D12Resource1* mQuadIB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW			mQuadVBView;
	D3D12_INDEX_BUFFER_VIEW				mQuadIBView;

	// Upload ��
	OpenLight::GPUUploadHeapWrap* mUploadHeap = nullptr;
	OpenLight::GPUDescriptorHeapWrap* mDescriptorHeap = nullptr;
	// �������
	RoamCamera* mCamera = nullptr;
	// CPUFrameResource
	std::vector<std::unique_ptr<DeferredShadingRenderGraphFrameResource>>	mCPUFrameResource;
	// ʱ����
	Timer								mTimer;
	// ���Դ
	CBPointLights						mPointLights;
	int									mSelectedLightIndex = 0;

	// ������ IBL
	EnvironmentMap						mEnvironmentMap;
	bool								mUseIBL = true;

	// ����
	std::shared_ptr<Terrain>			mTerrain = nullptr;


	float								mDeltaTime = 0.f;


};