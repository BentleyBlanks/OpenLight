#include "DescriptorAllocation.h"
#include "DescriptorAllocatorPage.h"
#include <cassert>

DescriptorAllocation::DescriptorAllocation()
	: mDescriptor{0}
	, mNumHandles(0)
	, mDescriptorSize(0)
	, mPage(nullptr)
{
}

DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor , uint32_t numHandles , uint32_t descriptorSize , std::shared_ptr<DescriptorAllocatorPage> page)
	: mDescriptor(descriptor)
	, mNumHandles(numHandles)
	, mDescriptorSize(descriptorSize)
	, mPage(page)
{
}

DescriptorAllocation::~DescriptorAllocation()
{
	Free();
}

DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation)
	: mDescriptor(allocation.mDescriptor)
	, mNumHandles(allocation.mNumHandles)
	, mDescriptorSize(allocation.mDescriptorSize)
	, mPage(std::move(allocation.mPage))
{
	allocation.mDescriptor.ptr = 0;
	allocation.mNumHandles     = 0;
	allocation.mDescriptorSize = 0;

}
DescriptorAllocation& DescriptorAllocation::operator=(DescriptorAllocation&& other)
{
	Free();

	mDescriptor     = other.mDescriptor;
	mNumHandles     = other.mNumHandles;
	mDescriptorSize = other.mDescriptorSize;
	mPage           = other.mPage;

	other.mDescriptor     = {0};
	other.mNumHandles     = 0;
	other.mDescriptorSize = 0;

	return *this;
}

bool DescriptorAllocation::IsNull() const
{
	return mDescriptor.ptr == 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetDescriptorHandle(uint32_t offset) const
{
	assert(offset < mNumHandles);
	return {mDescriptor.ptr + (mDescriptorSize * offset)};
}

uint32_t DescriptorAllocation::GetNumHandles() const
{
	return mNumHandles;
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocation::GetDescriptorAllocatorPage() const
{
	return mPage;
}

void DescriptorAllocation::Free()
{
	if (!IsNull() && mPage)
	{
		// Need FrameNumber
		mPage->Free(std::move(*this) , -1);

		mDescriptor.ptr = 0;
		mNumHandles     = 0;
		mDescriptorSize = 0;
		mPage.reset();
	}
}
