#pragma once
#include"Renderer.h"
#include"Camera.h"
#include "ObjMeshLoader.h"
#include"Timer.h"
#include<memory>

struct CBTrans
{
	XMFLOAT4X4 wvp;
	XMFLOAT4X4 world;
	XMFLOAT4X4 invTranspose;
};

class CubeRenderer :public Renderer
{
public:
	CubeRenderer();
	virtual ~CubeRenderer();
	virtual void Init(HWND hWnd) override;
	virtual void Render() override;

protected:
	void Update();
	void InitPostprocess();
	void FPS();
	WRL::ComPtr<ID3D12Resource1>		mTexture = nullptr;
	WRL::ComPtr<ID3D12Resource1>		mCBTrans = nullptr;
	CBTrans*							mCBTransGPUPtr = nullptr;
	WRL::ComPtr<ID3D12DescriptorHeap>	mSRVCBVHeap = nullptr;
	WRL::ComPtr<ID3D12DescriptorHeap>	mSamplerHeap = nullptr;
	WRL::ComPtr<ID3D12PipelineState>	mPSO = nullptr;
	WRL::ComPtr<ID3D12PipelineState>	mPSORGBA32 = nullptr;
	WRL::ComPtr<ID3D12RootSignature>	mRootSignature = nullptr;
	D3D12_VIEWPORT						mViewport;
	D3D12_RECT							mScissorRect;

	WRL::ComPtr<ID3D12Heap>				mUploadHeap;
	UINT								mUploadHeapSizeInBytes;

	int									mSamplerCount = 3;
	int									mSamplerIndex = 0;


	WRL::ComPtr<ID3D12DescriptorHeap>	mSceneColorRTVHeap		= nullptr;
	WRL::ComPtr<ID3D12DescriptorHeap>	mSceneColorSRVHeap		= nullptr;
	WRL::ComPtr<ID3D12RootSignature>	mPostprocessSignature	= nullptr;
	WRL::ComPtr<ID3D12PipelineState>	mPostprocessPSO			= nullptr;
	WRL::ComPtr<ID3D12Resource1>		mQuadVB					= nullptr;
	WRL::ComPtr<ID3D12Resource1>		mQuadIB					= nullptr;
	D3D12_VERTEX_BUFFER_VIEW			mQuadVBView;
	D3D12_INDEX_BUFFER_VIEW				mQuadIBView;
	WRL::ComPtr<ID3D12Resource1>		mSceneColorBuffer[AppConfig::NumFrames];


	// ÂþÓÎÏà»ú
	RoamCamera*							mCamera = nullptr;
	// Cube
	std::shared_ptr<ObjMesh>			mCubeMesh = nullptr;
	WRL::ComPtr<ID3D12Resource1>		mCubeMeshVB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW			mCubeMeshVBView;
	WRL::ComPtr<ID3D12Resource1>		mCubeMeshIB = nullptr;
	D3D12_INDEX_BUFFER_VIEW				mCubeMeshIBView;

	Timer								mTimer;
	bool								mGammaCorrect = false;
};

