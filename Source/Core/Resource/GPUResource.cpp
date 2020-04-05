#include"GPUResource.h"
#include "d3dx12.h"
#include<iostream>

#define DEFAULT_COMMON_SIZE 32
#define DEFAULT_SAMPLER_SIZE 8
OpenLight::GPUDescriptorRegister::GPUDescriptorRegister(WRL::ComPtr<ID3D12Device> device)
{
	D3D12_DESCRIPTOR_HEAP_DESC commonDesc = {};
	commonDesc.NumDescriptors = DEFAULT_COMMON_SIZE;
	commonDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	commonDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&commonDesc, IID_PPV_ARGS(&mCommonHeap)));
	commonDesc.NumDescriptors = DEFAULT_SAMPLER_SIZE;
	commonDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&commonDesc, IID_PPV_ARGS(&mSamplerHeap)));
}

void OpenLight::GPUDescriptorRegister::RegisterDescriptor(const std::string & tableName, GPUDescriptorTable * table, EGPUDescriptorTableType type)
{
	if (mRunning)
	{
		std::cout << "Exist Registering DescriptorTable!" << std::endl;
		assert(false);
	}
	auto p = mDescriptorTableHash.find(tableName);

	assert(p == mDescriptorTableHash.end());
	assert(table);

	mDescriptorTableHash.insert(tableName);
	mRunning = true;
	mRunningTable = table;
	mRunningTableType = type;

	if (mRunningTableType == EGPUDescriptorTableType_Comman)
		mRunningTable->offset = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCommonHeap->GetGPUDescriptorHandleForHeapStart(),
			mCommonOffset,
			mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	else
		mRunningTable->offset = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSamplerHeap->GetGPUDescriptorHandleForHeapStart(),
			mSamplerOffset,
			mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
}

void OpenLight::GPUDescriptorRegister::AddSRV(const std::string & resourceName,
	WRL::ComPtr<ID3D12Resource> resource, 
	const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
{
	assert(mRunning);
	auto& hash = mResourceHashs[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
	auto p = mResourceHashs[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].find(resourceName);
	assert(p != hash.end());
	
	mDevice->CreateShaderResourceView(resource.Get(), &srvDesc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mCommonHeap->GetCPUDescriptorHandleForHeapStart(),
			mCommonOffset,
			mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

	mCommonOffset++;
	mRunningTable->size++;
}

void OpenLight::GPUDescriptorRegister::AddCBV(const std::string & resourceName, WRL::ComPtr<ID3D12Resource> resource, D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc)
{
	assert(mRunning);
	auto& hash = mResourceHashs[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
	auto p = mResourceHashs[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].find(resourceName);
	assert(p != hash.end());
	cbvDesc.BufferLocation = resource->GetGPUVirtualAddress();
	mDevice->CreateConstantBufferView(&cbvDesc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mCommonHeap->GetCPUDescriptorHandleForHeapStart(),
			mCommonOffset,
			mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

	mCommonOffset++;
	mRunningTable->size++;
}

void OpenLight::GPUDescriptorRegister::AddUAV(const std::string & resourceName, 
	WRL::ComPtr<ID3D12Resource> resource,
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc,
	WRL::ComPtr<ID3D12Resource> resourceCount )
{
	assert(mRunning);
	auto& hash = mResourceHashs[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
	auto p = mResourceHashs[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].find(resourceName);
	assert(p != hash.end());
	mDevice->CreateUnorderedAccessView(resource.Get(), resourceCount.Get(), &uavDesc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mCommonHeap->GetCPUDescriptorHandleForHeapStart(),
			mCommonOffset,
			mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

	mCommonOffset++;
	mRunningTable->size++;
}

void OpenLight::GPUDescriptorRegister::AddSampler(const std::string & resourceName, D3D12_SAMPLER_DESC samplerDesc)
{
	assert(mRunning);
	auto& hash = mResourceHashs[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER];
	auto p = mResourceHashs[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].find(resourceName);
	assert(p != hash.end());
	mDevice->CreateSampler(&samplerDesc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mSamplerHeap->GetCPUDescriptorHandleForHeapStart(),
			mSamplerOffset,
			mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)));

	mSamplerOffset++;
	mRunningTable->size++;

}

void OpenLight::GPUDescriptorRegister::CommitRegisterDescriptorTable()
{
	mRunningTable->hasRegister = true;

	mRunning = false;
	mRunningTable = nullptr;

}
