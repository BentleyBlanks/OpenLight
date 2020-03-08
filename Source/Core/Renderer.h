#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <chrono>
#include "AppConfig.h"
using namespace Microsoft;
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

class Renderer
{
public:
	Renderer();

	virtual void Init(HWND hWnd);
	virtual void Render();
	void Resize(uint32_t width , uint32_t height);

	virtual uint64_t Signal(WRL::ComPtr<ID3D12CommandQueue> commandQueue, WRL::ComPtr<ID3D12Fence> Fence, uint64_t& FenceValue);
	virtual void WaitForFenceValue(WRL::ComPtr<ID3D12Fence> Fence,
		uint64_t FenceValue,
		HANDLE FenceHandle,
		std::chrono::milliseconds duration = std::chrono::milliseconds::max());
	virtual void Flush(WRL::ComPtr<ID3D12CommandQueue> commandQueue, WRL::ComPtr<ID3D12Fence> Fence, uint64_t& FenceValue, HANDLE FenceEvent);
protected:

	WRL::ComPtr<ID3D12Device2>             mDevice       = nullptr;
	WRL::ComPtr<ID3D12CommandQueue>        mCommandQueue = nullptr;
	WRL::ComPtr<IDXGISwapChain4>           mSwapChain    = nullptr;
	WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList  = nullptr;
	WRL::ComPtr<ID3D12DescriptorHeap>	   mRTVDescriptorHeap = nullptr;;
	WRL::ComPtr<ID3D12Resource>            mBackBuffer[AppConfig::NumFrames];
	WRL::ComPtr<ID3D12CommandAllocator>	   mCommandAllocator[AppConfig::NumFrames];

	uint32_t							   mRTVDescriptorSize;
	uint32_t							   mCurrentBackBufferIndex;


	// Synchronization Objects
	WRL::ComPtr<ID3D12Fence> mFence = nullptr;
	uint64_t	 mFenceValue = 0;
	uint64_t	 mFrameFenceValues[AppConfig::NumFrames] = {};
	HANDLE		 mFenceEvent;
};