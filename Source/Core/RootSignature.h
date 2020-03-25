#pragma once

#include <wrl.h>
#include <vector>
#include "d3dx12.h"

class RootSignature
{
public:
	RootSignature();
	RootSignature(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc , D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion);
	
	virtual ~RootSignature();

	void Destory();

	ID3D12RootSignature* GetRootSignature() const;

	void SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc , D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion);

	const D3D12_ROOT_SIGNATURE_DESC1& GetRootSignatureDesc() const;

	uint32_t GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;

	uint32_t GetNumDescriptors(uint32_t rootIndex) const;

private:
	D3D12_ROOT_SIGNATURE_DESC1 mRootSignatureDesc;
	ID3D12RootSignature* mRootSignature;

	// Need to know the number of descriptors per descriptor table.
	// A maximum of 32 descriptor tables are supported(since a 32-bit
	// mask is used to represent the descriptor tables in the root signature
	uint32_t mNumDescriptorsPerTable[32];

	// A bit mask that represents the root parameter indices that are
	// descriptor tables for samplers
	uint32_t mSamplerTableBitMask;
	// A bit mask that represents the root parameter indices that are
	// CBV, UAV, and SRV descriptor tables.
	uint32_t mDescriptorTableBitMask;
};