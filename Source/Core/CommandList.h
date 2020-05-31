#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <map>
#include <mutex>
#include <vector>

class Resource;
class ResourceStateTracker;
class UploadBuffer;
class DynamicDescriptorHeap;

class CommandList
{
public:
	CommandList(D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandList();

	D3D12_COMMAND_LIST_TYPE GetCommandListType() const;

	void TransitionBarrier(const Resource& resource, 
						   D3D12_RESOURCE_STATES stateAfter, 
						   UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, 
						   bool flushBarriers = false);

	void FlushResourceBarriers();

	void CopyResource(Resource& dstRes, const Resource& srcRes);

	void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
	template<typename T>
	void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, const T& data)
	{
		SetGraphicsDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
	}

	void SetShaderResourceView(uint32_t rootParameterIndex,
							   uint32_t descriptorOffset,
							   const Resource& resource,
							   D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
							   UINT firstSubresource = 0,
							   UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
							   const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr);

	void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0);

private:
	void TrackResource(const Resource& res);
	void TrackObject(ID3D12Object* object);

private:
	D3D12_COMMAND_LIST_TYPE m_D3D12CommandListType;

	ID3D12GraphicsCommandList2* m_D3D12CommandList;
	ID3D12CommandAllocator*		m_D3D12CommandAllocator;

	// For copy queues, it may be necessary to generate mips while loading texture
	// Mips can't be generated on copy queues but must be generated on compute or 
	// direct queues. In this case, a Compute command list is generated and executed
	// after the copy queue is finished uploading the first sub resource.
	std::shared_ptr<CommandList> m_ComputeCommandList;

	// Keep track of the currently bound root signatures to minimize root signature changes.
	ID3D12RootSignature* m_RootSignature;

	// Resource created in an upload heap. Useful for drawing of dynamic geometry
	// or for uploading constant buffer data that changes every draw call.
	std::unique_ptr<ResourceStateTracker> m_ResourceStateTracker;

	// Resource created in an upload heap. Useful for drawing of dynamic geometry
	// or for uploading constant buffer data that changes every draw call.
	std::unique_ptr<UploadBuffer> m_UploadBuffer;

	// The dynamic descriptor heap allows for descriptors to be staged before
	// being commited to the command list. Dynamic descriptors need to be 
	// committed before a Draw or Dispatch.
	std::unique_ptr<DynamicDescriptorHeap> m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	using TrackObjects = std::vector<ID3D12Object*>;
	TrackObjects m_TrackObjects;
};