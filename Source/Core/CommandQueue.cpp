#include "CommandQueue.h"
#include "Utility.h"
#include <cassert>
#include <chrono>

#ifdef max
#undef max
#endif

CommandQueue::CommandQueue(ID3D12Device2* device , D3D12_COMMAND_LIST_TYPE type)
	: mFenceValue(0)
	, mCommandListType(type)
	, mD3DDevice(device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type     = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(mD3DDevice->CreateCommandQueue(&desc , IID_PPV_ARGS(&mD3DCommandQueue)));
	ThrowIfFailed(mD3DDevice->CreateFence(mFenceValue , D3D12_FENCE_FLAG_NONE , IID_PPV_ARGS(&mD3D12Fence)));

	mFenceEvent = ::CreateEvent(NULL , FALSE , FALSE , NULL);
	assert(mFenceEvent && "Failed to create fence event handle.");
}

ID3D12GraphicsCommandList2* CommandQueue::GetCommandList()
{
	ID3D12CommandAllocator*		commandAllocator = nullptr;
	ID3D12GraphicsCommandList2* commandList      = nullptr;

	if (!mCommandAllocatorQueue.empty() && IsFenceComplete(mCommandAllocatorQueue.front().FenceValue))
	{
		commandAllocator = mCommandAllocatorQueue.front().commandAllocator;
		mCommandAllocatorQueue.pop();

		ThrowIfFailed(commandAllocator->Reset());
	}
	else
	{
		commandAllocator = CreateCommandAllocator();
	}

	if (!mCommandListQueue.empty())
	{
		commandList = mCommandListQueue.front();
		mCommandListQueue.pop();

		ThrowIfFailed(commandList->Reset(commandAllocator , nullptr));
	}
	else
	{
		commandList = CreateCommandList(commandAllocator);
	}

	// Associate the command allocator with the command list so that is can be
	// retrieved when the command list is executed.
	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator) , commandAllocator));

	return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList2* CommandList)
{
	CommandList->Close();

	ID3D12CommandAllocator* commandAllocator = nullptr;
	UINT dataSize = sizeof(commandAllocator);
	ThrowIfFailed(CommandList->GetPrivateData(__uuidof(ID3D12CommandAllocator) , &dataSize , &commandAllocator));

	ID3D12CommandList* const ppCommandLists[] = {CommandList};

	mD3DCommandQueue->ExecuteCommandLists(1 , ppCommandLists);
	uint64_t fenceValue = Signal();

	mCommandAllocatorQueue.emplace(CommandAllocatorEntry{fenceValue, commandAllocator});
	mCommandListQueue.push(CommandList);

	return fenceValue;
}

uint64_t CommandQueue::Signal()
{
	uint64_t FenceValueForSignal = ++mFenceValue;
	ThrowIfFailed(mD3DCommandQueue->Signal(mD3D12Fence , FenceValueForSignal));
	return FenceValueForSignal;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
	return mD3D12Fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
	if (mD3D12Fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(mD3D12Fence->SetEventOnCompletion(fenceValue , mFenceEvent));
		::WaitForSingleObject(mFenceEvent , static_cast<DWORD>(std::chrono::milliseconds::max().count()));
	}
}

void CommandQueue::Flush()
{
	uint64_t FenceValueForSignal = Signal();
	WaitForFenceValue(FenceValueForSignal);
}

ID3D12CommandQueue* CommandQueue::GetD3D12CommandQueue() const
{
	return mD3DCommandQueue;
}

ID3D12CommandAllocator* CommandQueue::CreateCommandAllocator()
{
	ID3D12CommandAllocator* commandAllocator = nullptr;
	ThrowIfFailed(mD3DDevice->CreateCommandAllocator(mCommandListType , IID_PPV_ARGS(&commandAllocator)));
	return commandAllocator;
}

ID3D12GraphicsCommandList2* CommandQueue::CreateCommandList(ID3D12CommandAllocator* Allocator)
{
	ID3D12GraphicsCommandList2* commandList = nullptr;
	ThrowIfFailed(mD3DDevice->CreateCommandList(0 , mCommandListType , Allocator , nullptr , IID_PPV_ARGS(&commandList)));

	return commandList;
}


