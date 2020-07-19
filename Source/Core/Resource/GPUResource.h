#pragma once

#include "Utility.h"
#include<d3d12.h>
#include"d3dx12.h"
#include<thread>
#include<map>
#include<array>
#include<unordered_set>
#include <wrl.h>

#include "App.h"
using namespace Microsoft;
namespace OpenLight
{

	class GPUDescriptorHeapWrap
	{
	public:
		struct GPUDescriptorHeapWrapIndex
		{
			GPUDescriptorHeapWrapIndex()
			{
				offset = 0;
				size   = 0;
				index  = 0;
				heap   = nullptr;
			}
			friend GPUDescriptorHeapWrap;
			UINT offset;
			UINT size;
			UINT index;
			D3D12_CPU_DESCRIPTOR_HANDLE	cpuHandle;
			D3D12_DESCRIPTOR_HEAP_TYPE  type;

			bool valid() const { return heap; }
		private:
			GPUDescriptorHeapWrap* heap = nullptr;
		};


		GPUDescriptorHeapWrap(ID3D12Device5* device);
		GPUDescriptorHeapWrapIndex Allocate(UINT size,D3D12_DESCRIPTOR_HEAP_TYPE type);

		void AddCBV(GPUDescriptorHeapWrapIndex* index, ID3D12Resource* resource, D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc);
		void AddSRV(GPUDescriptorHeapWrapIndex* index, ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc);
		void AddUAV(GPUDescriptorHeapWrapIndex* index, ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc,ID3D12Resource* counter = nullptr);
		void AddRTV(GPUDescriptorHeapWrapIndex* index, ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC rtvDesc);
		void AddDSV(GPUDescriptorHeapWrapIndex* index, ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc);
		void AddSampler(GPUDescriptorHeapWrapIndex* index, D3D12_SAMPLER_DESC samplerDesc);
	
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle(const GPUDescriptorHeapWrapIndex& index);

		ID3D12DescriptorHeap* GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
		{
			return mHeaps[type].Get();
		}
	protected:

		
		std::array<WRL::ComPtr<ID3D12DescriptorHeap>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mHeaps;
		std::array<UINT, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>								mOffsets;
		WRL::ComPtr<ID3D12Device>															mDevice;
	};
	using DescriptorIndex = GPUDescriptorHeapWrap::GPUDescriptorHeapWrapIndex;

	class GPUUploadHeapWrap
	{
	public:
		// 默认大小为 128 MB
		GPUUploadHeapWrap(ID3D12Device* device,UINT capacity = (1<<27));

		~GPUUploadHeapWrap()
		{
			mUploadHeap->Release();
		}


		bool createResource(ID3D12Device5*device, UINT sizeInBytes,D3D12_RESOURCE_DESC& desc, REFIID riid,_COM_Outptr_opt_  void **ppvResource);
		bool createResource(ID3D12Device5*device, UINT sizeInBytes, D3D12_RESOURCE_DESC& desc, ID3D12Resource* & resource);
		ID3D12Resource* createResource(ID3D12Device5*device, UINT sizeInBytes, D3D12_RESOURCE_DESC& desc)
		{
			if (mUploadOffsetInBytes + sizeInBytes >= mUploadCapacityInBytes)
			{
				assert(false);
				return nullptr;
			}

			ID3D12Resource* resource;
			ThrowIfFailed(device->CreatePlacedResource(
				mUploadHeap,
				mUploadOffsetInBytes,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&resource)));

			mUploadOffsetInBytes = PAD(mUploadOffsetInBytes + sizeInBytes, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			return resource;
		}
	protected:
		ID3D12Heap1*					mUploadHeap;
		UINT							mUploadCapacityInBytes;
		UINT							mUploadOffsetInBytes;
	};
	
	class GPUDynamicDescriptorHeapWrap
	{
	public:
		static GPUDynamicDescriptorHeapWrap* GetInstance();

		GPUDynamicDescriptorHeapWrap(ID3D12Device5* device);

		// CBV SRV UAV
		std::pair<CD3DX12_GPU_DESCRIPTOR_HANDLE,CD3DX12_CPU_DESCRIPTOR_HANDLE> 
			AllocateGPU(UINT size);

		// RTV DSV
		CD3DX12_CPU_DESCRIPTOR_HANDLE AllocateRTV(UINT size);
		CD3DX12_CPU_DESCRIPTOR_HANDLE AllocateDSV(UINT size);

		ID3D12DescriptorHeap* GetGPUDescriptorHeap() {
			return nullptr;
		}
		ID3D12DescriptorHeap* GetRTVDescriptorHeap() {
			return nullptr;
		}
		ID3D12DescriptorHeap* GetDSVDescriptorHeap()
		{
			return nullptr;
		}

		void Reset() {
			mOffsets[0] = 0;
			mOffsets[1] = 0;
			mOffsets[2] = 0;
		}
	protected:
		ID3D12DescriptorHeap* mGPUDescriptorHeap = nullptr;
		ID3D12DescriptorHeap* mRTVDescriptorHeap = nullptr;
		ID3D12DescriptorHeap* mDSVDescriptorHeap = nullptr;

		// 0: GPU
		// 1: RTV
		// 2: DSV
		std::array<UINT, 3>		mOffsets;
		ID3D12Device5* mDevice = nullptr;
	};
}

#if 0
#define RegisterGPUClass(ClassName)								\
	class ClassName##GPURegister :public OpenLight::GPURegister	\
	{															\
	public:														\
		static const char* strName() { return #ClassName;}		\
		static ClassName##GPURegister& getInstance()			\
		{														\
			static ClassName##GPURegister Instance;				\
			return Instance;									\
		}														\
	private:													\
	ClassName##GPURegister(){}									\
	ClassName##GPURegister& operator=(const ClassName##GPURegister) = delete; \
	};															\
	ClassName##GPURegister& GetRegisterGPUClassInstance()		\
	{	\
		return ClassName##GPURegister::getInstance();			\
	}

#define RegisterResource(Resource,Type)							\
	GetRegisterGPUClassInstance().Register##Type(#Resource)		

#define RegisterDescriptorTable(DescriptorTable,Type)			\
	GetRegisterGPUClassInstance().RegisterDescriptor(#DescriptorTable,&DescriptorTable,Type)

#define DescriptorCommonAdd(Resource,Desc,Type)					\
	GetRegisterGPUClassInstance().Add##Type(#Resource,Resource,Desc)

#define DescriptorCommonUAVExAdd(Resource,Desc,ResourceCounter) \
	GetRegisterGPUClassInstance().AddUAV(#Resource,Resource,Desc,ResourceCounter)

#define DescriptorSamplerAdd(Resource,Desc)								\
	GetRegisterGPUClassInstance().AddSampler(#Resource,Desc)

#define DescriptorCommit	GetRegisterGPUClassInstance().CommitRegisterDescriptorTable()
#endif