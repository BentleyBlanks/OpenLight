#include "DescriptorAllocatorPage.h"
#include "RenderHelper.h"
#include "Utility.h"

DescriptorAllocatorPage::DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type , uint32_t numDescriptors)
	: mHeapType(type)
	, mNumDescriptorsInHeap(numDescriptors)
{
	//auto device = RenderHelper::gDevice;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = mHeapType;
	heapDesc.NumDescriptors = mNumDescriptorsInHeap;
	
	ThrowIfFailed(RenderHelper::gDevice->CreateDescriptorHeap(&heapDesc , IID_PPV_ARGS(&mD3D12DescriptorHeap)));

	mBaseDescriptor                = mD3D12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	mDescriptorHandleIncrementSize = RenderHelper::gDevice->GetDescriptorHandleIncrementSize();
	mNumFreeHandles                = mNumDescriptorsInHeap;

	AddNewBlock(0 , mNumFreeHandles);
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocatorPage::GetHeapType() const
{
	return mHeapType;
}

bool DescriptorAllocatorPage::HasSpace(uint32_t numDescriptors) const
{
	return mFreeListBySize.lower_bound(numDescriptors) != mFreeListBySize.end();
}

uint32_t DescriptorAllocatorPage::NumFreeHandles() const
{
	return mNumFreeHandles;
}

DescriptorAllocation DescriptorAllocatorPage::Allocation(uint32_t numDescriptors)
{
	std::lock_guard<std::mutex> lock(mAllocationMutex);

	if (numDescriptors > mNumFreeHandles)
	{
		return DescriptorAllocation();
	}

	auto smallestBlockIt = mFreeListBySize.lower_bound(numDescriptors);
	if(smallestBlockIt == mFreeListBySize.end())
	{
		return DescriptorAllocation();
	}

	auto blockSize = smallestBlockIt->first;
	auto offsetIt  = smallestBlockIt->second;

	auto offset = offsetIt->first;

	mFreeListBySize.erase(smallestBlockIt);
	mFreeListByOffset.erase(offsetIt);

	auto newOffset    = offset + numDescriptors;
	auto newBlockSize = blockSize - numDescriptors;

	if (newBlockSize > 0)
	{
		AddNewBlock(newOffset , newBlockSize);
	}
	
	mNumFreeHandles -= numDescriptors;

	return DescriptorAllocation(CD3DX12_CPU_DESCRIPTOR_HANDLE(mBaseDescriptor , offset , mDescriptorHandleIncrementSize) ,
								numDescriptors ,
								mDescriptorHandleIncrementSize ,
								shared_from_this());
}

void DescriptorAllocatorPage::Free(DescriptorAllocation&& descriptor , uint64_t frameNumber)
{
	auto offset = ComputeOffset(descriptor.GetDescriptorHandle());
	std::lock_guard<std::mutex> lock(mAllocationMutex);

	// Don't add the block directly to the free list until the frame has completed.
	mStaleDescriptors.emplace(offset , descriptor.GetNumHandles() , frameNumber);
}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{
	std::lock_guard<std::mutex> lock(mAllocationMutex);

	while (!mStaleDescriptors.empty() && mStaleDescriptors.front().FrameNumber <= frameNumber)
	{
		auto staleDesciptor = mStaleDescriptors.front();
		auto offset         = staleDesciptor.Offset;
		auto numDescriptors = staleDesciptor.Size;

		FreeBlock(offset , numDescriptors);

		mStaleDescriptors.pop();
	}
}

uint32_t DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	return static_cast<uint32_t>(handle.ptr - mBaseDescriptor.ptr) / mDescriptorHandleIncrementSize;
}

void DescriptorAllocatorPage::AddNewBlock(uint32_t offset , uint32_t numDescriptors)
{
	auto offsetIt = mFreeListByOffset.emplace(offset , numDescriptors);
	auto SizeIt   = mFreeListBySize.emplace(numDescriptors , offset);

	offsetIt.first->second.FreeListBySizeIt = SizeIt;
}

void DescriptorAllocatorPage::FreeBlock(uint32_t offset , uint32_t numDescriptors)
{
	// Find the first element whose offset is greater than the specified offset.
	// This is the block that should appear after the block that is being freed.
	auto nextBlockIt = mFreeListByOffset.upper_bound(offset);

	auto preBlockIt = nextBlockIt;
	if (preBlockIt != mFreeListByOffset.begin())
	{
		preBlockIt--;
	}
	else
	{
		preBlockIt = mFreeListByOffset.end();
	}

	mNumFreeHandles += numDescriptors;

	if (preBlockIt != mFreeListByOffset.end() && offset == preBlockIt->first + preBlockIt->second.Size)
	{
		offset          = preBlockIt->first;
		numDescriptors += preBlockIt->second.Size;

		mFreeListBySize.erase(preBlockIt->second.Size);
		mFreeListByOffset.erase(preBlockIt);
	}
	
	AddNewBlock(offset , numDescriptors);
}
