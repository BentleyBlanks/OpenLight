#pragma once

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

class UploadBuffer
{
public:
	struct Allocation
	{
		void* CPU;
		D3D12_GPU_VIRTUAL_ADDRESS GPU;
	};

	explicit UploadBuffer(size_t PageSize = 2 * 1024 * 1024);

	size_t GetPageSize() const;

	Allocation Allocate(size_t sizeInBytes , size_t alignment);

	// Release all allocated pages. This should be done when is finished executing on the CommandQueue
	void Reset();

private:
	struct Page
	{
		Page(size_t sizeInBytes);
		~Page();

		bool HasSpace(size_t sizeInBytes , size_t alignment) const;

		// Allocate memory from the page.
		Allocation Allocate(size_t sizeInBytes , size_t alignment);

		void Reset();

	private:
		ID3D12Resource* mD3D12Resource = nullptr;

		void* mBasePointer = nullptr;
		size_t mOffset;
		size_t mPageSize;
		D3D12_GPU_VIRTUAL_ADDRESS mGPUPtr;
	};

	// A pool of memory pages
	using PagePool = std::deque<std::shared_ptr<Page>>;

	std::shared_ptr<Page> RequestPage();

	PagePool mPagePool;
	PagePool mAvailablePages;

	std::shared_ptr<Page> mCurrentPage;
	// The size of each page of memory
	size_t mPageSize;
};