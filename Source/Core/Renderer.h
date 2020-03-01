#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "AppConfig.h"
#include <DirectXMath.h>
#include "CommandQueue.h"

class Scene;

class Renderer
{
public:
	Renderer();

	void Init(HWND hWnd, Scene* scene);
	void Render();
	void Resize(uint32_t width , uint32_t height);

	void Flush();

	ID3D12Device2* GetDevice();
	CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE);

private:
	void TransitionResource(ID3D12GraphicsCommandList2* CommandList ,
							ID3D12Resource* Resource                ,
							D3D12_RESOURCE_STATES beForeState       ,
							D3D12_RESOURCE_STATES AfterState);

	void ClearRTV(ID3D12GraphicsCommandList2* CommandList , 
				 D3D12_CPU_DESCRIPTOR_HANDLE rtv         , 
				 FLOAT* ClearColor);

	void ClearDepth(ID3D12GraphicsCommandList2* CommandList ,
					D3D12_CPU_DESCRIPTOR_HANDLE dsv         ,
					FLOAT depth = 1.0f);

	void ResizeDepthBuffer(int width , int height);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentDSV() const;
	ID3D12Resource* GetCurrentBackBuffer() const;

	UINT Present();

private:
	ID3D12Device2*        mDevice      = nullptr;
	IDXGISwapChain4*      SwapChain    = nullptr;
	ID3D12DescriptorHeap* RTVDescriptorHeap = nullptr;;
	UINT				  RTVDescriptorSize;
	ID3D12Resource*       BackBuffer[AppConfig::NumFrames] = {};
	UINT				  mCurrentBackBufferIndex;
	uint64_t			  mFenceValues[AppConfig::NumFrames] = {};

	CommandQueue* mDirectQueue  = nullptr;
	CommandQueue* mComputeQueue = nullptr;
	CommandQueue* mCopyQueue    = nullptr;

	Scene* mScene = nullptr;
private:
	ID3D12Resource* mDepthBuffer   = nullptr;
	ID3D12DescriptorHeap* mDSVHeap = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT	   mScissorRect;

	float mFov;
	DirectX::XMMATRIX mModelMatrix;
	DirectX::XMMATRIX mViewMatrix;
	DirectX::XMMATRIX mProjectionMatrix;
};