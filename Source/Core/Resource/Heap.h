#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <chrono>
#include "Utility.h"
#include "d3dx12.h"
#include "AppConfig.h"
namespace OpenLight
{
	template<D3D12_HEAP_TYPE heapType,int memAlignment, D3D12_HEAP_FLAGS heapFlags>
	class FrameHeap
	{
	public:
		// Default: 16MB
		FrameHeap(ID3D12Device* device, size_t memoryInBytes = 16 * (1 << 20))
		{
			mAllMemoryInBytes = memoryInBytes;
			mAllocatedInBytes = 0;
			
			D3D12_HEAP_DESC heapDesc = {};
			heapDesc.Alignment			= memAlignment;
			heapDesc.SizeInBytes		= mAllMemoryInBytes;
			heapDesc.Flags				= heapFlags;
			heapDesc.Properties.Type	= heapType;
			heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

			ThrowIfFailed(device->CreateHeap(heapDesc, IID_PPV_ARGS(mHeap));
		}

		D3D12_HEAP_TYPE			getHeapType()			const		{ return heapType; }
		int						getAlignment()			const		{ return memAlignment;}
		int						getHeapFlag()			const		{ return heapFlags; }
		size_t					getAllMemoryInBytes()	const		{ return mAllMemoryInBytes; }
		size_t					getFreeMemoryInBytes()  const		{ return mAllMemoryInBytes - mAllocatedInBytes; }

		
	protected:
		ID3D12Heap*		mHeap;
		size_t			mAllMemoryInBytes;
		size_t			mAllocatedInBytes;

	};

	class UploadBufferHeap:public FrameHeap<
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS>
	{
	public:
		
		template<typename T>
		bool allocateUploadBuffer(ID3D12Device* device,
			UINT elementCount,
			Microsoft::WRL::ComPtr<ID3D12Resource>& resource)
		{
			UINT elementSize = PADCB(sizeof(T));

			if (mAlignmentOffset + elementSize * elementCount > mAllMemoryInBytes)
				return false;

			ThrowIfFailed(device->CreatePlacedResource(mHeap,
				mAlignmentOffset,
				CD3DX12_RESOURCE_DESC::Buffer(elementCount * elementSize),
				D3D12_RESOURCE_STATE_DEPTH_READ,
				nullptr,
				IID_PPV_ARGS(resource)));

			mAlignmentOffset = PAD(mAlignmentOffset + elementCount * elementSize, getAlignment());

			return true;
		}
		
	protected:
		size_t			mAlignmentOffset = 0;
	};

}