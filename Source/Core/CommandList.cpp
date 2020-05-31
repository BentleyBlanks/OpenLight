#include "CommandList.h"
#include "d3dx12.h"
#include "ResourceStateTracker.h"
#include "UploadBuffer.h"
#include "DynamicDescriptorHeap.h"

D3D12_COMMAND_LIST_TYPE CommandList::GetCommandListType() const
{
	return m_D3D12CommandListType;
}

void CommandList::TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource, bool flushBarriers)
{
	auto d3d12Resource = resource.GetD3D12Resource();
	if (d3d12Resource)
	{
		// The "before" state is not important. It will be resolved by the resource state tracker.
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12Resource, D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource);
		m_ResourceStateTracker->ResourceBarrier(barrier);
	}

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::FlushResourceBarriers()
{
	m_ResourceStateTracker->FlushResourceBarriers(*this);
}

void CommandList::CopyResource(Resource& dstRes, const Resource& srcRes)
{
	TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

	FlushResourceBarriers();

	m_D3D12CommandList->CopyResource(dstRes.GetD3D12Resource(), srcRes.GetD3D12Resource());

	TrackResource(dstRes);
	TrackResource(srcRes);
}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	auto heapAllocation = m_UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	memcpy(heapAllocation.CPU, bufferData, sizeInBytes);

	m_D3D12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllocation.GPU);
}

void CommandList::SetShaderResourceView(uint32_t rootParameterIndex, 
										uint32_t descriptorOffset, 
										const Resource& resource, 
										D3D12_RESOURCE_STATES stateAfter, 
										UINT firstSubresource, 
										UINT numSubresources, 
										const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
	if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (uint32_t i = 0; i < numSubresources; i++)
		{
			TransitionBarrier(resource, stateAfter, firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(resource, stateAfter);
	}

	m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, resource.GetShaderResourceView(srv));

	TrackResource(resource);
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
	FlushResourceBarriers();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
	{
		m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	m_D3D12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::TrackResource(const Resource& res)
{
	TrackObject(res.GetD3D12Resource());
}

void CommandList::TrackObject(ID3D12Object* object)
{
	m_TrackObjects.push_back(object);
}
