#include "UploadBuffer.h"
#include "d3dx12.h"
#include <new>
#include "RenderHelper.h"
#include "Utility.h"
#include <DirectXMath.h>

UploadBuffer::UploadBuffer(size_t PageSize)
	:mPageSize(PageSize)
{
}

size_t UploadBuffer::GetPageSize() const
{
	return size_t();
}

UploadBuffer::Allocation UploadBuffer::Allocate(size_t sizeInBytes , size_t alignment)
{
	if (sizeInBytes > mPageSize)	throw std::bad_alloc();

	if (!mCurrentPage || !mCurrentPage->HasSpace(sizeInBytes , alignment))
	{
		mCurrentPage = RequestPage();
	}

	return mCurrentPage->Allocate(sizeInBytes , alignment);
}

void UploadBuffer::Reset()
{
	mCurrentPage    = nullptr;
	mAvailablePages = mPagePool;

	for (auto page : mAvailablePages)
	{
		page->Reset();
	}
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
	std::shared_ptr<Page> page;

	if (!mAvailablePages.empty())
	{
		page = mAvailablePages.front();
		mAvailablePages.pop_front();
	}
	else
	{
		page = std::make_shared<Page>(mPageSize);
		mPagePool.push_back(page);
	}
	return page;
}

UploadBuffer::Page::Page(size_t sizeInBytes)
	: mPageSize(sizeInBytes)
	, mOffset(0)
	, mBasePointer(nullptr)
	, mGPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
	ThrowIfFailed(RenderHelper::gDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) ,
																  D3D12_HEAP_FLAG_NONE ,
																  &CD3DX12_RESOURCE_DESC::Buffer(mPageSize) ,
																  D3D12_RESOURCE_STATE_GENERIC_READ,
																  nullptr ,
																  IID_PPV_ARGS(&mD3D12Resource)));

	mGPUPtr = mD3D12Resource->GetGPUVirtualAddress();
	mD3D12Resource->Map(0 , nullptr , &mBasePointer);
}

UploadBuffer::Page::~Page()
{
	mD3D12Resource->Unmap(0 , nullptr);
	mBasePointer = nullptr;
	mOffset      = 0;
	mGPUPtr      = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

template<typename T>
inline T AlignUpWithMask(T value , size_t mask)
{
	return (T)(size_t)((value + mask) & ~mask);
}

template<typename T>
inline T AlignUp(T value , size_t alignment)
{
	return AlignDownWithMask(value , alignment - 1);
}

bool UploadBuffer::Page::HasSpace(size_t sizeInBytes , size_t alignment) const
{
	size_t alignedSize   = AlignUp(sizeInBytes , alignment);
	size_t alignedOffset = AlignUp(mOffset , alignment);

	return alignedSize + alignedOffset <= mPageSize;
}


// this isn't safe-thread. but we will not used same uploadbuffer in different thread.
UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t sizeInBytes , size_t alignment)
{
	if (!HasSpace(sizeInBytes , alignment))
	{
		throw std::bad_alloc();
	}

	size_t alignedSize = AlignUp(sizeInBytes , alignment);
	mOffset = AlignUp(mOffset , alignment);

	Allocation allocation;
	allocation.CPU = static_cast<uint8_t*>(mBasePointer) + mOffset;
	allocation.GPU = mGPUPtr + mOffset;

	mOffset += alignedSize;

	return allocation;
}

void UploadBuffer::Page::Reset()
{
	mOffset = 0;
}
