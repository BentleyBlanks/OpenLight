#pragma once

#include <d3d12.h>
#include <cstdint>
#include <memory>

class DescriptorAllocatorPage;

class DescriptorAllocation
{
public:
	DescriptorAllocation();

	DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor , uint32_t numHandles , uint32_t descriptorSize , std::shared_ptr<DescriptorAllocatorPage> page);

	// The destructor will automatically free the allocation.
	~DescriptorAllocation();

	// Copies are not allowed.
	DescriptorAllocation(const DescriptorAllocation&) = delete;
	DescriptorAllocation& operator=(const DescriptorAllocation& other) = delete;

	// Move is allowed
	DescriptorAllocation(DescriptorAllocation&& allocation);
	DescriptorAllocation& operator=(DescriptorAllocation&& other);

	bool IsNull() const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t offset = 0) const;

	// Get the number of (consecutive) handles for this allocation.
	uint32_t GetNumHandles() const;

	// Get the heap that this allocation came from
	// (For internal use only).
	std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage() const;

private:
	void Free();

	D3D12_CPU_DESCRIPTOR_HANDLE mDescriptor;
	// The number of descriptors in this allocation.
	uint32_t mNumHandles;
	// The offset to the next descriptors.
	uint32_t mDescriptorSize;

	// A pointer back to the original page where this allocation came from.
	std::shared_ptr<DescriptorAllocatorPage> mPage;
};