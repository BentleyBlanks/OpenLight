#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <chrono>
#include "AppConfig.h"
#include "GUI/imgui.h"
#include "GUI/imgui_impl_win32.h"
#include "GUI/imgui_impl_dx12.h"
using namespace Microsoft;
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

class App;
class Renderer
{
public:
	friend App;
	Renderer();
	virtual ~Renderer();

	virtual void Init(HWND hWnd);
	virtual void Render();
	virtual void Resize(uint32_t width , uint32_t height);

	virtual uint64_t Signal(ID3D12CommandQueue* commandQueue, ID3D12Fence* Fence, uint64_t& FenceValue);
	virtual void WaitForFenceValue(ID3D12Fence* Fence,
		uint64_t FenceValue,
		HANDLE FenceHandle,
		std::chrono::milliseconds duration = std::chrono::milliseconds::max());
	virtual void Flush(ID3D12CommandQueue* commandQueue, ID3D12Fence* Fence, uint64_t& FenceValue, HANDLE FenceEvent);
	virtual void Flush(ID3D12CommandQueue* commandQueue, ID3D12Fence* Fence, uint64_t& FenceValue);
	virtual void renderGUI();
protected:


	ID3D12Device5*				mDevice       = nullptr;
	ID3D12CommandQueue*			mCommandQueue = nullptr;
	IDXGISwapChain4*			mSwapChain    = nullptr;
	ID3D12GraphicsCommandList*	mCommandList  = nullptr;
	ID3D12DescriptorHeap*		mRTVDescriptorHeap = nullptr;;
	ID3D12Resource*				mBackBuffer[AppConfig::NumFrames];
	ID3D12CommandAllocator*		mCommandAllocator[AppConfig::NumFrames];


	ID3D12DescriptorHeap*				   mPd3dSrvDescHeap = nullptr;

	uint32_t							   mRTVDescriptorSize;
	uint32_t							   mCurrentBackBufferIndex = 0;
	HWND								   mHWND;


	// Synchronization Objects
	ID3D12Fence* mFence = nullptr;
	uint64_t	 mFenceValue = 0;
	uint64_t	 mFrameFenceValues[AppConfig::NumFrames] = {};
	HANDLE		 mFenceEvent;
};

