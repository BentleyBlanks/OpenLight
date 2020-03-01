#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <queue>

class CommandQueue
{
public:
	CommandQueue(ID3D12Device2* device , D3D12_COMMAND_LIST_TYPE type);

	ID3D12GraphicsCommandList2* GetCommandList();

	uint64_t ExecuteCommandList(ID3D12GraphicsCommandList2* CommandList);
	
	uint64_t Signal();
	bool	 IsFenceComplete(uint64_t fenceValue);
	void	 WaitForFenceValue(uint64_t fenceValue);
	void	 Flush();

	ID3D12CommandQueue* GetD3D12CommandQueue() const;

protected:
	ID3D12CommandAllocator* CreateCommandAllocator();
	ID3D12GraphicsCommandList2* CreateCommandList(ID3D12CommandAllocator* Allocator);

private:
	// Keep track of command allocators that are "in-flight"
	// CommandAllocator can't be reused unless the command 
	// stored in command allocator has finished execute on the command queue
	struct CommandAllocatorEntry
	{
		uint64_t FenceValue;
		ID3D12CommandAllocator* commandAllocator;
	};

	using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
	using CommandListQueue      = std::queue<ID3D12GraphicsCommandList2*>;

	D3D12_COMMAND_LIST_TYPE mCommandListType;
	ID3D12Device2*			mD3DDevice;
	ID3D12CommandQueue*		mD3DCommandQueue;
	ID3D12Fence*			mD3D12Fence;
	HANDLE					mFenceEvent;
	uint64_t				mFenceValue;

	CommandAllocatorQueue mCommandAllocatorQueue;
	CommandListQueue	  mCommandListQueue;
};