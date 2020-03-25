#include "RenderHelper.h"
#include <d3d12.h>

ID3D12Device2*	 RenderHelper::gDevice    = nullptr;
IDXGISwapChain4* RenderHelper::gSwapChain = nullptr;

UINT RenderHelper::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
	return gDevice->GetDescriptorHandleIncrementSize(heapType);
}
