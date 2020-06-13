#include "FrameResource.h"

OpenLight::CPUFrameResource::CPUFrameResource(
	ID3D12Device5 * device, 
	ID3D12CommandAllocator*	cmdListAlloc,
	UINT uploadCapacityInBytes)
{
	CPUResourceUploadHeap = std::make_unique<GPUUploadHeapWrap>(device, uploadCapacityInBytes);
	CmdListAlloc = cmdListAlloc;
}

OpenLight::CPUFrameResource::~CPUFrameResource()
{

}
