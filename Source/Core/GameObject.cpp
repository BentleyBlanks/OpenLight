#include "GameObject.h"
#include <DirectXMath.h>
#include "Utility.h"
#include "App.h"
#include <d3dx12.h>

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

void GameObject::Load(ID3D12Device2* Device , ID3D12GraphicsCommandList2* CommandList)
{
	ID3D12Resource* intermediateVertexBuffer = nullptr;
	UpdateBufferResource(Device                    ,
						 CommandList               ,
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
						 CommandList              ,
						 &mIndexBuffer            ,
						 &intermediateIndexBuffer ,
						 _countof(gIndices)       ,
						 sizeof(WORD)             ,
						 gIndices);

	mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
	mIndexBufferView.Format         = DXGI_FORMAT_R16_UINT;
	mIndexBufferView.SizeInBytes    = sizeof(gIndices);
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

	D3D12_HEAP_PROPERTIES HeapProperties;
	HeapProperties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.CreationNodeMask     = 1;
	HeapProperties.VisibleNodeMask      = 1;

	D3D12_RESOURCE_DESC Desc;
	Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
	Desc.Alignment          = 0;
	Desc.Width              = bufferSize;
	Desc.Height             = 1;
	Desc.DepthOrArraySize   = 1;
	Desc.MipLevels          = 1;
	Desc.Format             = DXGI_FORMAT_UNKNOWN;
	Desc.SampleDesc.Count   = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.Flags              = flags;
	ThrowIfFailed(Device->CreateCommittedResource(&HeapProperties                ,
												  D3D12_HEAP_FLAG_NONE           ,
												  &Desc                          ,
												  D3D12_RESOURCE_STATE_COPY_DEST ,
												  nullptr                        ,
												  IID_PPV_ARGS(pDestinationResource)));

	if (bufferData)
	{
		D3D12_HEAP_PROPERTIES IntermediateHeapProperties;
		IntermediateHeapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
		IntermediateHeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		IntermediateHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		IntermediateHeapProperties.CreationNodeMask     = 1;
		IntermediateHeapProperties.VisibleNodeMask      = 1;

		D3D12_RESOURCE_DESC IntermediateDesc;
		IntermediateDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
		IntermediateDesc.Alignment          = 0;
		IntermediateDesc.Width              = bufferSize;
		IntermediateDesc.Height             = 1;
		IntermediateDesc.DepthOrArraySize   = 1;
		IntermediateDesc.MipLevels          = 1;
		IntermediateDesc.Format             = DXGI_FORMAT_UNKNOWN;
		IntermediateDesc.SampleDesc.Count   = 1;
		IntermediateDesc.SampleDesc.Quality = 0;
		IntermediateDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		IntermediateDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(Device->CreateCommittedResource(&IntermediateHeapProperties       ,
													  D3D12_HEAP_FLAG_NONE              ,
													  &IntermediateDesc                 ,
													  D3D12_RESOURCE_STATE_GENERIC_READ ,
													  nullptr                           ,
													  IID_PPV_ARGS(pIntermediateResource)));
		
		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData      = bufferData;
		subResourceData.RowPitch   = bufferSize;
		subResourceData.SlicePitch = subResourceData.RowPitch;
		
		UpdateSubresources(CommandList , *pDestinationResource , *pIntermediateResource , 0 , 0 , 1 , &subResourceData);
	}
}
