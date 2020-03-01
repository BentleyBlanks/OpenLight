#pragma once

#include <d3d12.h>
#include "Shader.h"

class GameObject
{
public:
	GameObject();

	void Load(ID3D12Device2* Device , ID3D12GraphicsCommandList2* CommandList);

	void SetShader(const Shader* material);
	const Shader* GetShader() const;

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
	D3D12_INDEX_BUFFER_VIEW  GetIndexBufferView() const;

	int GetNumIndices() const;

private:
	void UpdateBufferResource(ID3D12Device2* device                   ,
							  ID3D12GraphicsCommandList2* CommandList ,
							  ID3D12Resource** pDestinationResource   ,
							  ID3D12Resource** pIntermediateResource  ,
							  size_t numElements                      ,
							  size_t elementSize                      ,
							  const void* bufferData                  ,
							  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

private:
	ID3D12Resource* mVertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

	ID3D12Resource* mIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

	const Shader* Material = nullptr;
};