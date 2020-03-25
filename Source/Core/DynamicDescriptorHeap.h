#pragma once

#include "d3dx12.h"
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <queue>
#include <functional>

class CommandList;
class RootSignature;

// The purpose of the class is to allocate GPU visible descriptors
// that are used for binding CBV,SRV,UAV and Samplers to GPU Pipeline
// for rendering or compute invocations.
// 
// Since only a single CBV_SRV_UAV descriptor heap and a single SAMPLER descriptor
// heap can be bound to the commandList at the same time.
// The class also ensures that the currently bound descriptor heap has a sufficient number of 
// descriptors to commit all of the satged descriptors before a draw or dispatch command is executed.
class DynamicDescriptorHeap
{
public:
	DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType , uint32_t numDescriptorsPerHeap = 1024);

	virtual ~DynamicDescriptorHeap();

	// Stage a contiguous range of CPU visible descriptors.
	// Descriptors are not copied to the GPU visible descriptor heap until
	// the CommitStagedDescriptors function is called.
	void StageDescriptors(uint32_t rootParameterIndex , 
						  uint32_t offset , 
						  uint32_t numDescriptors , 
						  const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptors);

	/*
		Copy all of the staged descriptors to the GPU visible descriptor heap and 
		bind the descriptor heap and the descriptor tables to the command list.
		The passed-in function object is used to set the GPU visible descriptors
		on the command list.
	*/
	void CommitStagedDescriptors(CommandList& commandList , 
								 std::function<void(ID3D12GraphicsCommandList* , UINT , D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc);

	void CommitStagedDescriptorsForDraw(CommandList& commandList);
	void CommitStagedDescriptorsForDispatch(CommandList& commandList);

	/*
		Copies a single CPU visible descriptor to a GPU visible descriptor heap
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor(CommandList& commandList , D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor);

	// Parse the root signature to determine which root parameter contain
	// descriptor tables and determine the number of descriptors needed for
	// each table.
	void ParseRootSignature(const RootSignature& rootSignature);

	/*
		Rest used descriptor. This should only be done if any descriptors
		that are being referenced by a command list has finished
	*/
	void Reset();

private:
	// Request a descriptor heap if one is available
	ID3D12DescriptorHeap* RequestDescriptorHeap();
	// Create a new descriptor heap of no descriptor heap is available
	ID3D12DescriptorHeap* CreateDescriptorHeap();

	uint32_t ComputeStaleDescriptorCount();

	/*
		The maximum number of descriptor tables per root signature
		A 32-bit mask is used to keep track of the root parameter indices that
		are descriptor tables
	*/
	static const uint32_t MaxDescriptorTables = 32;

	/*
		A structure that represents a descriptor table entry in the root signature.
	*/
	struct DescriptorTableCache
	{
		DescriptorTableCache()
			: mNumDescriptors(0)
			, mBaseDescriptor(nullptr)
		{
		}

		void Reset()
		{
			mNumDescriptors = 0;
			mBaseDescriptor = nullptr;
		}

		uint32_t mNumDescriptors;
		D3D12_CPU_DESCRIPTOR_HANDLE* mBaseDescriptor;
	};

	D3D12_DESCRIPTOR_HEAP_TYPE mDescriptorHeapType;
	uint32_t mNumDescriptorsPerHeap;
	uint32_t mDescriptorHandleIncrementSize;

	// The descriptor handle cache.
	std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> mDescriptorHandleCache;

	// Descriptor handle cache per descriptor table.
	DescriptorTableCache mDescriptorTableCache[MaxDescriptorTables];

	// Each bit in the bit mask represents the index in the root signature
	// that contains a descriptor table.
	uint32_t mDescriptorTableBitMask;

	// Each bit set in the bit mask represents a descriptor table
	// in the root signature that has changed since the last time
	// the descriptors were copied.
	uint32_t mStaleDescriptorTableBitMask;

	using DescriptorHeapPool = std::queue<ID3D12DescriptorHeap>;
	DescriptorHeapPool mDescriptorHeapPool;
	DescriptorHeapPool mAvailableDescriptorHeaps;

	ID3D12DescriptorHeap* mCurrentDescriptorHeap = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mCurrentCPUDescriptorHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mCurrentGPUDescriptorHandle;

	uint32_t mNumFreeHandles;
};