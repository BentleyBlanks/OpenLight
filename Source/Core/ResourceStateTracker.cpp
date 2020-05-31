#include "ResourceStateTracker.h"
#include "d3dx12.h"
#include "CommandList.h"
#include "Resource.h"
#include <cassert>

std::mutex ResourceStateTracker::ms_GlobalMutex;
bool ResourceStateTracker::ms_IsLocked = false;
ResourceStateTracker::ResourceStateMap ResourceStateTracker::ms_GlobalResourceState;

ResourceStateTracker::ResourceStateTracker()
{
}

ResourceStateTracker::~ResourceStateTracker()
{
}

void ResourceStateTracker::ResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier)
{
	if (barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
	{
		const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier = barrier.Transition;

		const auto iter = m_FinalResourceState.find(transitionBarrier.pResource);
		if (iter != m_FinalResourceState.end())
		{
			auto& resourceState = iter->second;

			// If the known final state of the resource is different...
			if (transitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !resourceState.SubresourceState.empty())
			{
				for (auto subresourceState : resourceState.SubresourceState)
				{
					if (transitionBarrier.StateAfter != subresourceState.second)
					{
						D3D12_RESOURCE_BARRIER newBarrier = barrier;
						newBarrier.Transition.Subresource = subresourceState.first;
						newBarrier.Transition.StateBefore = subresourceState.second;
						m_ResourceBarriers.push_back(newBarrier);
					}
				}
			}
			else
			{
				auto finalState = resourceState.GetSubresourceState(transitionBarrier.Subresource);
				if (transitionBarrier.StateAfter != finalState)
				{
					D3D12_RESOURCE_BARRIER newBarrier = barrier;
					newBarrier.Transition.StateBefore = finalState;
					m_ResourceBarriers.push_back(newBarrier);
				}
			}
		}
		else // In this case, the resource is being used on the command list for the first time.
		{
			m_PendingResourceBarriers.push_back(barrier);
		}

		// Push the final known state (possibly replacing the previously known state for the subresource).
		m_FinalResourceState[transitionBarrier.pResource].SetSubresourceState(transitionBarrier.Subresource, transitionBarrier.StateAfter);
	}
	else
	{
		// just push non-transition barriers to the resource barriers array.
		m_ResourceBarriers.push_back(barrier);
	}
}

void ResourceStateTracker::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource)
{
	if (resource)
	{
		ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource));
	}
}

void ResourceStateTracker::TransitionResource(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource)
{
	TransitionResource(resource.GetD3D12Resource(), stateAfter, subResource);
}

void ResourceStateTracker::UAVBarrier(const Resource* resource)
{
	ID3D12Resource* pResource = resource != nullptr ? resource->GetD3D12Resource() : nullptr;

	ResourceBarrier(CD3DX12_RESOURCE_BARRIER::UAV(pResource));
}

void ResourceStateTracker::AliasBarrier(const Resource* resourceBefore, const Resource* resourceAfter)
{
	ID3D12Resource* pResourceBefore = resourceBefore != nullptr ? resourceBefore->GetD3D12Resource() : nullptr;
	ID3D12Resource* pResourceAfter  = resourceAfter	 != nullptr ? resourceAfter->GetD3D12Resource()  : nullptr;

	ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Aliasing(pResourceBefore, pResourceAfter));
}

uint32_t ResourceStateTracker::FlushPendingResourceBarriers(CommandList& commandList)
{
	assert(ms_IsLocked);

	ResourceBarriers resourceBarriers;
	resourceBarriers.reserve(m_PendingResourceBarriers.size());

	for (auto pendingBarrier : m_PendingResourceBarriers)
	{
		// only transition barriers should be pending..
		if (pendingBarrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
		{
			auto pendingTransition = pendingBarrier.Transition;
			const auto& iter = ms_GlobalResourceState.find(pendingTransition.pResource);
			if (iter != ms_GlobalResourceState.end())
			{
				auto& resourceState = iter->second;

				if (pendingTransition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !resourceState.SubresourceState.empty())
				{
					for (auto subResourceState : resourceState.SubresourceState)
					{
						D3D12_RESOURCE_BARRIER newBarrier = pendingBarrier;
						newBarrier.Transition.Subresource = subResourceState.first;
						newBarrier.Transition.StateBefore = subResourceState.second;
						resourceBarriers.push_back(newBarrier);
					}
				}
				else
				{
					auto globalState = (iter->second).GetSubresourceState(pendingTransition.Subresource);
					if (pendingTransition.StateAfter != globalState)
					{
						pendingBarrier.Transition.StateBefore = globalState;
						resourceBarriers.push_back(pendingBarrier);
					}
				}
			}
		}
	}

	UINT numBarriers = static_cast<UINT>(resourceBarriers.size());
	if (numBarriers > 0)
	{
		auto d3d12CommandList = commandList.GetGraphicsCommandList();
		d3d12CommandList->ResourceBarrier(numBarriers, resourceBarriers.data());
	}

	m_PendingResourceBarriers.clear();

	return numBarriers;
}

void ResourceStateTracker::FlushResourceBarriers(CommandList& commandList)
{
	UINT numBarriers = static_cast<UINT>(m_ResourceBarriers.size());
	if (numBarriers > 0)
	{
		auto d3d12CommandList = commandList.GetGraphicsCommandList();
		d3d12CommandList->ResourceBarrier(numBarrier, m_ResourceBarriers.data());
		m_ResourceBarriers.clear();
	}
}

void ResourceStateTracker::CommitFinalResourceStates()
{
	assert(ms_IsLocked);

	// Commit final resource states to the global resource state array (map).
	for (const auto& resourceState : m_FinalResourceState)
	{
		ms_GlobalResourceState[resourceState.first] = resourceState.second;
	}

	m_FinalResourceState.clear();
}

void ResourceStateTracker::Reset()
{
	m_PendingResourceBarriers.clear();
	m_ResourceBarriers.clear();
	m_FinalResourceState.clear();
}

void ResourceStateTracker::Lock()
{
	ms_GlobalMutex.lock();
	ms_IsLocked = true;
}

void ResourceStateTracker::Unlock()
{
	ms_IsLocked = false;
	ms_GlobalMutex.unlock();
}

void ResourceStateTracker::AddGlobalResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
{
	if (resource != nullptr)
	{
		std::lock_guard<std::mutex> lock(ms_GlobalMutex);
		ms_GlobalResourceState[resource].SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, state);
	}
}

void ResourceStateTracker::RemoveGlobalResourceState(ID3D12Resource* resource)
{
	if (resource != nullptr)
	{
		std::lock_guard<std::mutex> lock(ms_GlobalMutex);
		ms_GlobalResourceState.erase(resource);
	}
}
