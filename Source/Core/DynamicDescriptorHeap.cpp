#include "DynamicDescriptorHeap.h"
#include "RenderHelper.h"
#include "RootSignature.h"
#include "Utility.h"
#include <cassert>
#include <stdexcept>

DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType , uint32_t numDescriptorsPerHeap)
	: mDescriptorHeapType(heapType)
	, mNumDescriptorsPerHeap(numDescriptorsPerHeap)
	, mDescriptorTableBitMask(0)
	, mStaleDescriptorTableBitMask(0)
	, mCurrentCPUDescriptorHandle(D3D12_DEFAULT)
	, mCurrentGPUDescriptorHandle(D3D12_DEFAULT)
	, mNumFreeHandles(0)
{
	mDescriptorHandleIncrementSize = RenderHelper::GetDescriptorHandleIncrementSize(heapType);

	// Allocate space for staging CPU visible descriptors
	mDescriptorHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(mNumDescriptorsPerHeap);
}

DynamicDescriptorHeap::~DynamicDescriptorHeap()
{
}

void DynamicDescriptorHeap::StageDescriptors(uint32_t rootParameterIndex , uint32_t offset , uint32_t numDescriptors , const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptors)
{
	if (numDescriptors > mNumDescriptorsPerHeap || rootParameterIndex >= MaxDescriptorTables)
	{
		throw std::bad_alloc();
	}

	DescriptorTableCache& descriptorTableCache = mDescriptorTableCache[rootParameterIndex];

	// Check that the number of descriptors to copy does not exceed the number
	// of descriptors expected in the descriptor table.
	if ((offset + numDescriptors) > descriptorTableCache.mNumDescriptors)
	{
		throw std::length_error("Number of descriptors exceeds the number of descriptors in the descriptor tables");
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = (descriptorTableCache.mBaseDescriptor + offset);
	for (uint32_t i = 0; i < numDescriptors; ++i)
	{
		dstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(srcDescriptors , i , mDescriptorHandleIncrementSize);
	}

	mStaleDescriptorTableBitMask |= (1 << rootParameterIndex);
}

void DynamicDescriptorHeap::CommitStagedDescriptors(CommandList& commandList , std::function<void(ID3D12GraphicsCommandList* , UINT , D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc)
{
	uint32_t numDescriptorsToCommit = ComputeStaleDescriptorCount();
	if (numDescriptorsToCommit <= 0)	return;

	auto device = RenderHelper::gDevice;
	auto dx12GraphicsCommitList = commandList.GetGraphicsCommandList();
	assert(dx12GraphicsCommitList != nullptr);

	if (!mCurrentDescriptorHeap || mNumFreeHandles < numDescriptorsToCommit)
	{
		mCurrentDescriptorHeap = RequestDescriptorHeap();
		mCurrentCPUDescriptorHandle = mCurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		mCurrentGPUDescriptorHandle = mCurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		mNumFreeHandles = mNumDescriptorsPerHeap;

		commandList.SetDescriptorHeap(mDescriptorHeapType, mCurrentDescriptorHeap);

		// when updating the descriptor heap on the command list,
		// all descriptor cache tables must be recopied to the new descriptor heap
		// (not just the stale descriptor tables).
		mStaleDescriptorTableBitMask = mDescriptorTableBitMask;
	}

	DWORD rootIndex;
	while (_BitScanForward(&rootIndex, mStaleDescriptorTableBitMask))
	{
		UINT numSrcDescriptors = mDescriptorTableCache[rootIndex].mNumDescriptors;
		D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorHandles = mDescriptorTableCache[rootIndex].mBaseDescriptor;

		D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[] = { mCurrentCPUDescriptorHandle };
		UINT pDestDescriptorRangeSizes[] = { numSrcDescriptors };

		RenderHelper::gDevice->CopyDescriptors(1, 
											   pDestDescriptorRangeStarts, 
											   pDestDescriptorRangeSizes, 
											   numSrcDescriptors, 
											   pSrcDescriptorHandles, 
											   nullptr, 
											   mDescriptorHeapType);

		setFunc(dx12GraphicsCommitList, rootIndex, mCurrentGPUDescriptorHandle);

		mCurrentCPUDescriptorHandle.Offset(numSrcDescriptors, mDescriptorHandleIncrementSize);
		mCurrentGPUDescriptorHandle.Offset(numSrcDescriptors, mDescriptorHandleIncrementSize);
		mNumFreeHandles -= numSrcDescriptors;
	}
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(CommandList& commandList)
{
	CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(CommandList& commandList)
{
	CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::CopyDescriptor(CommandList& commandList , D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor)
{
	if (!mCurrentDescriptorHeap || mNumFreeHandles < 1)
	{
		mCurrentDescriptorHeap = RequestDescriptorHeap();
		mCurrentCPUDescriptorHandle = mCurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		mCurrentGPUDescriptorHandle = mCurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		mNumFreeHandles = mNumDescriptorsPerHeap;

		commandList.SetDescriptorHeap(mDescriptorHeapType, mCurrentDescriptorHeap);

		mStaleDescriptorTableBitMask = mDescriptorTableBitMask;
	}

	auto device = RenderHelper::gDevice;

	D3D12_GPU_DESCRIPTOR_HANDLE hGPU = mCurrentGPUDescriptorHandle;
	device->CopyDescriptorsSimple(1, mCurrentCPUDescriptorHandle, cpuDescriptor, mDescriptorHeapType);

	mCurrentCPUDescriptorHandle.Offset(1, mDescriptorHandleIncrementSize);
	mCurrentGPUDescriptorHandle.Offset(1, mDescriptorHandleIncrementSize);
	mNumFreeHandles -= 1;

	return hGPU;
}

void DynamicDescriptorHeap::ParseRootSignature(const RootSignature& rootSignature)
{
	// if the root signature changes, all descriptors must be (re)bound to the command list.
	mStaleDescriptorTableBitMask = 0;

	const auto& rootSignatureDesc = rootSignature.GetRootSignatureDesc();
	
	// Get a bit mask that represents the root parameter indices that match the
	// descriptor heap type for this dynamic descriptor heap.
	mDescriptorTableBitMask = rootSignature.GetDescriptorTableBitMask(mDescriptorHeapType);

	uint32_t descriptorTableBitMask = mDescriptorTableBitMask;
	uint32_t currentOffset = 0;
	DWORD rootIndex;
	// _BitScanForward scan a bitfield from least-significant bit to most-significant bit and
	// store the position of the first set bit in the next argument.
	while (_BitScanForward(&rootIndex , descriptorTableBitMask) && rootIndex < rootSignatureDesc.NumParameters)
	{
		uint32_t numDescriptors = rootSignature.GetNumDescriptors(rootIndex);

		DescriptorTableCache& descriptorTableCache = mDescriptorTableCache[rootIndex];
		descriptorTableCache.mNumDescriptors = numDescriptors;
		descriptorTableCache.mBaseDescriptor = mDescriptorHandleCache.get() + currentOffset;

		currentOffset += numDescriptors;

		// Flip the descriptor table bit so it's not scanned againfor the currednt index.
		descriptorTableBitMask ^= (1 << rootIndex);
	}

	assert(currentOffset < mNumDescriptorsPerHeap && "The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum number of descriptors per descriptor heap");
}

void DynamicDescriptorHeap::Reset()
{
	mAvailableDescriptorHeaps = mDescriptorHeapPool;
	mCurrentDescriptorHeap->Release();
	mCurrentCPUDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	mCurrentGPUDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	mNumFreeHandles = 0;
	mDescriptorTableBitMask = 0;
	mStaleDescriptorTableBitMask = 0;

	for (int i = 0; i < MaxDescriptorTables; i++)
	{
		mDescriptorTableCache[i].Reset();
	}
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::RequestDescriptorHeap()
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;

	if (!mAvailableDescriptorHeaps.empty())
	{
		descriptorHeap = mAvailableDescriptorHeaps.front();
		mAvailableDescriptorHeaps.pop();
	}
	else
	{
		descriptorHeap = CreateDescriptorHeap();
		mDescriptorHeapPool.push(descriptorHeap);
	}

	return descriptorHeap;
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::CreateDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = mDescriptorHeapType;
	descriptorHeapDesc.NumDescriptors = mNumDescriptorsPerHeap;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	ThrowIfFailed(RenderHelper::gDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

uint32_t DynamicDescriptorHeap::ComputeStaleDescriptorCount()
{
	uint32_t numStaleDescriptors = 0;
	DWORD i;
	DWORD staleDescriptorsBitMask = mStaleDescriptorTableBitMask;

	while (_BitScanForward(&i, numStaleDescriptors))
	{
		numStaleDescriptors += mDescriptorTableCache[i].mNumDescriptors;
		staleDescriptorsBitMask ^= (1 << i);
	}

	return numStaleDescriptors;
}
