#pragma once

#include "DescriptorAllocation.h"

#include <d3d12.h>
#include <wrl.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>

#include "d3dx12.h"

class DescriptorAllocatorPage : public std::enable_shared_from_this<DescriptorAllocatorPage>
{
public:
	DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type , uint32_t numDescriptors);

	D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;

	/*
		Check to see if this descriptor page has a contiguous block of descriptors
		large enough to satisfy the request.
	*/
	bool HasSpace(uint32_t numDescriptors) const;

	/*
		Get the number of available handles in the heap.
	*/
	uint32_t NumFreeHandles() const;

	/*
		Allocate a number of descriptors from this descriptor heap.
		If the allocation cannot be satisfied. then a Null descriptor is returned.
	*/
	DescriptorAllocation Allocation(uint32_t numDescriptors);

	/*
		Return a descriptor back to the heap.
	*/
	void Free(DescriptorAllocation&& descriptorHandle , uint64_t frameNumber);

	void ReleaseStaleDescriptors(uint64_t frameNumber);

protected:
	uint32_t ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);

	void AddNewBlock(uint32_t offset , uint32_t numDescriptors);

	// Free a block of descriptors
	// This will also merge free blocks in the free list to form larger blocks
	void FreeBlock(uint32_t offset , uint32_t numDescriptors);

private:
	struct FreeBlockInfo;

	using FreeListByOffset = std::map<uint32_t , FreeBlockInfo>;
	using FreeListBySize  = std::multimap<uint32_t , FreeListByOffset::iterator>;

	struct FreeBlockInfo
	{
		FreeBlockInfo(uint32_t size)
			: Size(size)
		{
		}

		uint32_t Size;

		FreeListBySize::iterator FreeListBySizeIt;
	};
	
	struct StaleDescriptorInfo
	{
		StaleDescriptorInfo(uint32_t offset , uint32_t size , uint64_t frameNumber)
			: Offset(offset)
			, Size(size)
			, FrameNumber(frameNumber)
		{

		}

		// The offset within the descriptor heap.
		uint32_t Offset;

		// The number of descriptors.
		uint32_t Size;

		// The frame number that the descriptor was freed.
		uint64_t FrameNumber;
	};

	// Stale Descriptors are queued for release until the frame 
	// that they were freed has completed.
	using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

	FreeListByOffset	 mFreeListByOffset;
	FreeListBySize		 mFreeListBySize;
	StaleDescriptorQueue mStaleDescriptors;

	ID3D12DescriptorHeap*         mD3D12DescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE    mHeapType;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBaseDescriptor;
	uint32_t                      mDescriptorHandleIncrementSize;
	uint32_t                      mNumDescriptorsInHeap;
	uint32_t                      mNumFreeHandles;
	std::mutex					  mAllocationMutex;
};
