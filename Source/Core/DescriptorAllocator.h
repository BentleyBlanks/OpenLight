#pragma once

#include "d3dx12.h"
#include <cstdint>
#include <mutex>
#include <set>
#include <vector>
#include "DescriptorAllocation.h"

class DescriptorAllocatorPage;

// http://diligentgraphics.com/diligent-engine/architecture/d3d12/variable-size-memory-allocations-manager/
class DescriptorAllocator
{
public:
	DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type , uint32_t numDescriptorsPerHeap = 256);
	virtual ~DescriptorAllocator();

	// Allocate a number of contiguous descriptors from a CPU visiable
	DescriptorAllocation Allocate(uint32_t numDescriptors = 1);

	// when the frame has completed, the stale descriptors can be release
	void ReleaseStaleDescriptors(uint64_t frameNumber);

private:
	using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

	// Create a new heap with a specific number of descriptors
	std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

	D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;
	uint32_t mNumDescriptorsPerHeap;

	DescriptorHeapPool mHeapPool;
	//Indices of avaliable heaps in the heap pool
	std::set<size_t> mAvailableHeaps;

	std::mutex mAllocationMutex;
};