#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

class RenderHelper
{
public:
	static ID3D12Device2*   gDevice;
	static IDXGISwapChain4* gSwapChain;

	static UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
};