#include"GPUResource.h"
#include "d3dx12.h"
#include<iostream>

#define DEFAULT_COMMON_SIZE 64
#define DEFAULT_SAMPLER_SIZE 8

OpenLight::GPUDescriptorHeapWrap::GPUDescriptorHeapWrap(ID3D12Device5* device)
{
	mDevice = device;
	for (auto& p : mOffsets)
		p = 0;


	D3D12_DESCRIPTOR_HEAP_DESC desc = {};

	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = DEFAULT_COMMON_SIZE;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV])));
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV])));
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV])));
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	desc.NumDescriptors = DEFAULT_SAMPLER_SIZE;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER])));

}

OpenLight::GPUDescriptorHeapWrap::GPUDescriptorHeapWrapIndex OpenLight::GPUDescriptorHeapWrap::Allocate(UINT size,
	D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
		assert(mOffsets[type] + size <= DEFAULT_SAMPLER_SIZE);
	else
		assert(mOffsets[type] + size <= DEFAULT_COMMON_SIZE);

	GPUDescriptorHeapWrapIndex index;
	index.offset = mOffsets[type];
	index.size = size;
	index.index = 0;
	index.type = type;

	index.cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeaps[type]->GetCPUDescriptorHandleForHeapStart(),
		mOffsets[type],
		mDevice->GetDescriptorHandleIncrementSize(type));
	mOffsets[type] += size;
	index.heap = this;
	return index;
}



void OpenLight::GPUDescriptorHeapWrap::AddCBV(GPUDescriptorHeapWrapIndex * index,
	ID3D12Resource * resource,
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc)
{
	if (index->index >= index->size)
		ErrorBox("AddCBV");
	assert(index->type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	assert(index->index < index->size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		index->cpuHandle,
		index->index,
		mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	cbvDesc.BufferLocation = resource->GetGPUVirtualAddress();
	mDevice->CreateConstantBufferView(&cbvDesc, handle);
	index->index++;
}

void OpenLight::GPUDescriptorHeapWrap::AddSRV(GPUDescriptorHeapWrapIndex * index, ID3D12Resource * resource, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
{
	assert(index->type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	assert(index->index < index->size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		index->cpuHandle,
		index->index,
		mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	mDevice->CreateShaderResourceView(resource, &srvDesc, handle);
	index->index++;
}

void OpenLight::GPUDescriptorHeapWrap::AddUAV(GPUDescriptorHeapWrapIndex * index, ID3D12Resource * resource, D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc, ID3D12Resource* counter)
{
	assert(index->type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	assert(index->index < index->size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		index->cpuHandle,
		index->index,
		mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	mDevice->CreateUnorderedAccessView(resource, counter, &uavDesc, handle);
	index->index++;
}

void OpenLight::GPUDescriptorHeapWrap::AddRTV(GPUDescriptorHeapWrapIndex * index, ID3D12Resource * resource, D3D12_RENDER_TARGET_VIEW_DESC rtvDesc)
{
	assert(index->type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	assert(index->index < index->size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		index->cpuHandle,
		index->index,
		mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

	mDevice->CreateRenderTargetView(resource, &rtvDesc, handle);
	index->index++;
}

void OpenLight::GPUDescriptorHeapWrap::AddDSV(GPUDescriptorHeapWrapIndex * index, ID3D12Resource * resource, D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc)
{
	assert(index->type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	assert(index->index < index->size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		index->cpuHandle,
		index->index,
		mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));

	mDevice->CreateDepthStencilView(resource, &dsvDesc, handle);
	index->index++;
}

void OpenLight::GPUDescriptorHeapWrap::AddSampler(GPUDescriptorHeapWrapIndex* index, D3D12_SAMPLER_DESC samplerDesc)
{
	assert(index->type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	assert(index->index < index->size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		index->cpuHandle,
		index->index,
		mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));
	mDevice->CreateSampler(&samplerDesc, handle);
	index->index++;
}

D3D12_GPU_DESCRIPTOR_HANDLE OpenLight::GPUDescriptorHeapWrap::GPUHandle(const GPUDescriptorHeapWrapIndex& index)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(mHeaps[index.type]->GetGPUDescriptorHandleForHeapStart(),
		index.offset,
		mDevice->GetDescriptorHandleIncrementSize(index.type));

	return handle;
}

OpenLight::GPUUploadHeapWrap::GPUUploadHeapWrap(ID3D12Device * device, UINT capacity)
{
	mUploadCapacityInBytes = capacity;
	mUploadOffsetInBytes   = 0;

	D3D12_HEAP_DESC heapDesc                 = {};
	heapDesc.Alignment                       = 0; // 64kb ¶ÔÆë
	heapDesc.SizeInBytes                     = PAD(capacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	heapDesc.Properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
	heapDesc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapDesc.Properties.CreationNodeMask     = 0;
	heapDesc.Properties.VisibleNodeMask      = 0;
	heapDesc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

	ThrowIfFailed(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&mUploadHeap)));
}

bool OpenLight::GPUUploadHeapWrap::createResource(ID3D12Device5 * device,
	UINT sizeInBytes, 
	D3D12_RESOURCE_DESC & desc,
	REFIID riid, 
	_COM_Outptr_opt_ void ** ppvResource)
{
	if (mUploadOffsetInBytes + sizeInBytes >= mUploadCapacityInBytes)
	{

		return false;
	}

	ThrowIfFailed(device->CreatePlacedResource(
		mUploadHeap,
		mUploadOffsetInBytes,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		riid,
		ppvResource));
	mUploadOffsetInBytes = PAD(mUploadOffsetInBytes + sizeInBytes, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	return true;
}

bool OpenLight::GPUUploadHeapWrap::createResource(ID3D12Device5 * device, UINT sizeInBytes, D3D12_RESOURCE_DESC & desc, ID3D12Resource *& resource)
{
	if (mUploadOffsetInBytes + sizeInBytes >= mUploadCapacityInBytes)
	{

		return false;
	}

	ThrowIfFailed(device->CreatePlacedResource(
		mUploadHeap,
		mUploadOffsetInBytes,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)));
	mUploadOffsetInBytes = PAD(mUploadOffsetInBytes + sizeInBytes, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	return true;
}
