#include "DynamicDescriptorHeap.h"
#include "RenderHelper.h"
#include "RootSignature.h"
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
}

void DynamicDescriptorHeap::CommitStagedDescriptors(CommandList& commandList , std::function<void(ID3D12GraphicsCommandList* , UINT , D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc)
{
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(CommandList& commandList)
{
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(CommandList& commandList)
{
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::CopyDescriptor(CommandList& commandList , D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor)
{
	return D3D12_GPU_DESCRIPTOR_HANDLE();
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
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::RequestDescriptorHeap()
{
	return nullptr;
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::CreateDescriptorHeap()
{
	return nullptr;
}

uint32_t DynamicDescriptorHeap::ComputeStaleDescriptorCount()
{
	return uint32_t();
}
