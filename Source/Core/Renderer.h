#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "AppConfig.h"

class Renderer
{
public:
	Renderer();

	void Init(HWND hWnd);
	void Render();
	void Resize(uint32_t width , uint32_t height);

private:
	ID3D12Device2*             Device       = nullptr;
	ID3D12CommandQueue*        CommandQueue = nullptr;
	IDXGISwapChain4*           SwapChain    = nullptr;
	ID3D12GraphicsCommandList* CommandList  = nullptr;
	ID3D12DescriptorHeap*	   RTVDescriptorHeap = nullptr;;
	ID3D12Resource*            BackBuffer[AppConfig::NumFrames];
	ID3D12CommandAllocator*	   CommandAllocator[AppConfig::NumFrames];

	uint32_t RTVDescriptorSize;
	uint32_t CurrentBackBufferIndex;

	// Synchronization Objects
	ID3D12Fence* Fence = nullptr;
	uint64_t	 FenceValue = 0;
	uint64_t	 FrameFenceValues[AppConfig::NumFrames] = {};
	HANDLE		 FenceEvent;
};