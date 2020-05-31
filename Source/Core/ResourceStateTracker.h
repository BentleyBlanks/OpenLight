#pragma once

#include <d3d12.h>
#include <mutex>
#include <map>
#include <unordered_map>
#include <vector>

class CommandList;
class Resource;

class ResourceStateTracker
{
public:
	ResourceStateTracker();
	virtual ~ResourceStateTracker();

	/*
		Push a resource barrier to the resource state tracker.
	*/
	void ResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier);

	/*
		Push a transition resource barrier to the resource state tracker.
	*/
	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void TransitionResource(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	/*
		Push a UAV resource barrier for the given resource
	*/
	void UAVBarrier(const Resource* resource = nullptr);

	/*
		Push an alising barrier for the given resource.
	*/
	void AliasBarrier(const Resource* resourceBefore, const Resource* resourceAfter);

	/*
		Flush any pending resource barriers to the command list.
	*/
	uint32_t FlushPendingResourceBarriers(CommandList& commandList);

	/*
		Flush any (non-pending) resource barriers that have been pushed to the resource state tracker
	*/
	void FlushResourceBarriers(CommandList& commandList);

	/*
		Commit final resource states to the global resource state map.
		This must be called when the command list is closed.
	*/
	void CommitFinalResourceStates();

	/*
		Reset state tracking. This must be dne when the command list is reset.
	*/
	void Reset();

	/*
		The global state must be locked before flushing pending resource barriers
		and commiting the final resource state to the global resource state .
		This ensures consistency of the global resource state between command list
		executions.
	*/
	static void Lock();

	/*
		Unlocks the global resource state after the final states have been committed
		to the global resource state array.
	*/
	static void Unlock();

	/*
		Add a resource with a given state to the glboal resource state array (map).
		This should be done when the resource is created for the first time.
	*/
	static void AddGlobalResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);

	/*	
		Remove a resource from the global resource state array (map).
		This should only be done when the resource is destroyed.
	*/
	static void RemoveGlobalResourceState(ID3D12Resource* resource);

private:
	using ResourceBarriers = std::vector<D3D12_RESOURCE_BARRIER>;

	/*
		Panding resource transitions are committed before a command list
		is executed on the command queue. This guarantees that resources will
		be in the expected state at the beginning of a command list.
	*/
	ResourceBarriers m_PendingResourceBarriers;

	/*
		Resource barriers that need to be committed to the command list.
	*/
	ResourceBarriers m_ResourceBarriers;

	// Tracks the state of a particular resource and all of its subresources.
	struct ResourceState
	{
		explicit ResourceState(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON)
			:State(state)
		{

		}

		void SetSubresourceState(UINT subresource, D3D12_RESOURCE_STATES state)
		{
			if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
			{
				State = state;
				SubresourceState.clear();
			}
			else
			{
				SubresourceState[subresource] = state;
			}
		}

		D3D12_RESOURCE_STATES GetSubresourceState(UINT subresource) const
		{
			D3D12_RESOURCE_STATES state = State;

			const auto iter = SubresourceState.find(subresource);
			if (iter != SubresourceState.end())
			{
				state = iter->second;
			}

			return state;
		}

		D3D12_RESOURCE_STATES State;
		std::map<UINT, D3D12_RESOURCE_STATES> SubresourceState;
	};
	using ResourceStateMap = std::unordered_map<ID3D12Resource*, ResourceState>;
	
	/*
		The final (last known state) of the resources within a command list.
		The final resource state is committed to the global resource state when the
		command list is closed but before it is executed on the command queue.
	*/
	ResourceStateMap m_FinalResourceState;

	/*	
		The global resource state array (map) stores the state of a resource
		between command list execution.
	*/
	static ResourceStateMap ms_GlobalResourceState;

	// The mutex protects shared access to the GlobalResourceState map
	static std::mutex ms_GlobalMutex;
	static bool ms_IsLocked;
};