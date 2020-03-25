#include "DescriptorAllocator.h"
#include "DescriptorAllocatorPage.h"

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type , uint32_t numDescriptorsPerHeap)
	: mHeapType(type)
	, mNumDescriptorsPerHeap(numDescriptorsPerHeap)
{

}

DescriptorAllocator::~DescriptorAllocator()
{
}

DescriptorAllocation DescriptorAllocator::Allocate(uint32_t numDescriptors)
{
	std::lock_guard<std::mutex> lock(mAllocationMutex);

	DescriptorAllocation allocation;

	for (auto iter = mAvailableHeaps.begin(); iter != mAvailableHeaps.end(); iter++)
	{
		auto allocatorPage = mHeapPool[*iter];

		allocation = allocatorPage->Allocation(numDescriptors);

		if (allocatorPage->NumFreeHandles() == 0)
		{
			iter = mAvailableHeaps.erase(iter);
		}

		if (!allocation.IsNull())
		{
			break;
		}
	}

	// no available heap could satisty the requested number of descriptors
	if (allocation.IsNull())
	{
		mNumDescriptorsPerHeap = std::max(mNumDescriptorsPerHeap , numDescriptors);
		auto newPage = CreateAllocatorPage();
		
		allocation = newPage->Allocation(numDescriptors);
	}

	return allocation;
}

void DescriptorAllocator::ReleaseStaleDescriptors(uint64_t frameNumber)
{
	std::lock_guard<std::mutex> lock(mAllocationMutex);

	for (size_t i = 0; i < mHeapPool.size(); i++)
	{
		auto page = mHeapPool[i];

		page->ReleaseStaleDescriptors(frameNumber);

		if (page->NumFreeHandles() > 0)
		{
			mAvailableHeaps.insert(i);
		}
	}
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
	auto newPage = std::make_shared<DescriptorAllocatorPage>(mHeapType , mNumDescriptorsPerHeap);

	mHeapPool.emplace_back(newPage);
	mAvailableHeaps.insert(mHeapPool.size() - 1);

	return newPage;
}
