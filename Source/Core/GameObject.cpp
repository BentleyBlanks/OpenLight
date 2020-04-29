#include "GameObject.h"
#include <DirectXMath.h>
#include "Utility.h"
#include "App.h"
#include "d3dx12.h"
#include "CommandQueue.h"

struct VertexPosColor
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Color;
};

static VertexPosColor gVertices[8] =
{
	{DirectX::XMFLOAT3{-1.0f, -1.0f, -1.0f}, DirectX::XMFLOAT3{ 0.0f,  0.0f,  0.0f}},
	{DirectX::XMFLOAT3{-1.0f,  1.0f, -1.0f}, DirectX::XMFLOAT3{ 0.0f,  1.0f,  0.0f}},
	{DirectX::XMFLOAT3{ 1.0f,  1.0f, -1.0f}, DirectX::XMFLOAT3{ 1.0f,  1.0f,  0.0f}},
	{DirectX::XMFLOAT3{ 1.0f, -1.0f, -1.0f}, DirectX::XMFLOAT3{ 1.0f,  0.0f,  0.0f}},
	{DirectX::XMFLOAT3{-1.0f, -1.0f,  1.0f}, DirectX::XMFLOAT3{ 0.0f,  0.0f,  1.0f}},
	{DirectX::XMFLOAT3{-1.0f,  1.0f,  1.0f}, DirectX::XMFLOAT3{ 0.0f,  1.0f,  1.0f}},
	{DirectX::XMFLOAT3{ 1.0f,  1.0f,  1.0f}, DirectX::XMFLOAT3{ 1.0f,  1.0f,  1.0f}},
	{DirectX::XMFLOAT3{ 1.0f, -1.0f,  1.0f}, DirectX::XMFLOAT3{ 1.0f,  0.0f,  1.0f}},
};

static WORD gIndices[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

GameObject::GameObject()
{
}

void GameObject::Load(ID3D12Device2* Device , CommandQueue* commandQueue)
{
	auto CmdList = commandQueue->GetCommandList();

	ID3D12Resource* intermediateVertexBuffer = nullptr;
	UpdateBufferResource(Device                    ,
						 CmdList                   ,
						 &mVertexBuffer            ,
						 &intermediateVertexBuffer ,
						 _countof(gVertices)       ,
						 sizeof(VertexPosColor)    ,
						 gVertices);

	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes    = sizeof(gVertices);
	mVertexBufferView.StrideInBytes  = sizeof(VertexPosColor);

	ID3D12Resource* intermediateIndexBuffer = nullptr;
	UpdateBufferResource(Device                   ,
						 CmdList                  ,
						 &mIndexBuffer            ,
						 &intermediateIndexBuffer ,
						 _countof(gIndices)       ,
						 sizeof(WORD)             ,
						 gIndices);

	mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
	mIndexBufferView.Format         = DXGI_FORMAT_R16_UINT;
	mIndexBufferView.SizeInBytes    = sizeof(gIndices);

	auto FenceValue = commandQueue->ExecuteCommandList(CmdList);
	commandQueue->WaitForFenceValue(FenceValue);
}

void GameObject::SetShader(const Shader* material)
{
	Material = material;
}

const Shader* GameObject::GetShader() const
{
	return Material;
}

D3D12_VERTEX_BUFFER_VIEW GameObject::GetVertexBufferView() const
{
	return mVertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW GameObject::GetIndexBufferView() const
{
	return mIndexBufferView;
}

int GameObject::GetNumIndices() const
{
	return _countof(gIndices);
}

void GameObject::UpdateBufferResource(ID3D12Device2* Device                   ,
									  ID3D12GraphicsCommandList2* CommandList , 
									  ID3D12Resource** pDestinationResource   , 
									  ID3D12Resource** pIntermediateResource  , 
									  size_t numElements                      , 
									  size_t elementSize                      , 
									  const void* bufferData                  , 
									  D3D12_RESOURCE_FLAGS flags /*= D3D12_RESOURCE_FLAG_NONE*/)
{
	size_t bufferSize = numElements * elementSize;

	ThrowIfFailed(Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) ,
												  D3D12_HEAP_FLAG_NONE         ,
												  &CD3DX12_RESOURCE_DESC::Buffer(bufferSize , flags) ,
												  D3D12_RESOURCE_STATE_COMMON  ,
												  nullptr                      ,
												  IID_PPV_ARGS(pDestinationResource)));

	if (bufferData)
	{
		ThrowIfFailed(Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) ,
													  D3D12_HEAP_FLAG_NONE              ,
													  &CD3DX12_RESOURCE_DESC::Buffer(bufferSize) ,
													  D3D12_RESOURCE_STATE_GENERIC_READ ,
													  nullptr                           ,
													  IID_PPV_ARGS(pIntermediateResource)));
		
		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData      = bufferData;
		subResourceData.RowPitch   = bufferSize;
		subResourceData.SlicePitch = subResourceData.RowPitch;
		
		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource   = *pDestinationResource;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
			barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			CommandList->ResourceBarrier(1 , &barrier);
		}

		UpdateSubresources(CommandList , *pDestinationResource , *pIntermediateResource , 0 , 0 , 1 , &subResourceData);

		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource   = *pDestinationResource;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COMMON;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			
			CommandList->ResourceBarrier(1 , &barrier);
		}
	}
}