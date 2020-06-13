#pragma once
#include "GPUResource.h"
#include<memory>
namespace OpenLight
{
	// 每帧都需要在 CPU 端更新的资源
	// 由于不同的 Demo 所需的资源不同
	// 所以使用继承来使用该类
	class CPUFrameResource
	{
	public:
		CPUFrameResource(
			ID3D12Device5*					device,
			ID3D12CommandAllocator*			cmdListAlloc,
			UINT							uploadCapacityInBytes = (1 << 27));
		CPUFrameResource(const CPUFrameResource&) = delete;
		CPUFrameResource& operator=(const CPUFrameResource&) = delete;
		~CPUFrameResource();


		uint64_t							Fence                  = 0;
	protected:
		// CPU端更新的数据都创造在上传端
		std::unique_ptr<GPUUploadHeapWrap>  CPUResourceUploadHeap  = nullptr;

		ID3D12CommandAllocator*				CmdListAlloc           = nullptr;
	
		
		
		

	};
}