#pragma once
#include "GPUResource.h"
#include<memory>
namespace OpenLight
{
	// ÿ֡����Ҫ�� CPU �˸��µ���Դ
	// ���ڲ�ͬ�� Demo �������Դ��ͬ
	// ����ʹ�ü̳���ʹ�ø���
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
		// CPU�˸��µ����ݶ��������ϴ���
		std::unique_ptr<GPUUploadHeapWrap>  CPUResourceUploadHeap  = nullptr;

		ID3D12CommandAllocator*				CmdListAlloc           = nullptr;
	
		
		
		

	};
}