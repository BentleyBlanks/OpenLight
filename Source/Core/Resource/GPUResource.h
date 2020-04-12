#pragma once

#include "Utility.h"
#include<d3d12.h>
#include<thread>
#include<map>
#include<array>
#include<unordered_set>
#include <wrl.h>
using namespace Microsoft;
namespace OpenLight
{

	class GPUDescriptorHeapWrap
	{
	public:
		struct GPUDescriptorHeapWrapIndex
		{
			UINT offset;
			UINT size;
			UINT index;
			D3D12_CPU_DESCRIPTOR_HANDLE	handle;
			D3D12_DESCRIPTOR_HEAP_TYPE  type;
		};


		GPUDescriptorHeapWrap(WRL::ComPtr<ID3D12Device> device);

		GPUDescriptorHeapWrapIndex Allocate(UINT size,D3D12_DESCRIPTOR_HEAP_TYPE type);

		void AddCBV(GPUDescriptorHeapWrapIndex* index, ID3D12Resource* resource, D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc);
		void AddSRV(GPUDescriptorHeapWrapIndex* index, ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc);
		void AddUAV(GPUDescriptorHeapWrapIndex* index, ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc,ID3D12Resource* counter = nullptr);
		void AddRTV(GPUDescriptorHeapWrapIndex* index, ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC rtvDesc);
		void AddDSV(GPUDescriptorHeapWrapIndex* index, ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc);
		void AddSampler(GPUDescriptorHeapWrapIndex* index, D3D12_SAMPLER_DESC samplerDesc);
	protected:

		
		std::array<WRL::ComPtr<ID3D12DescriptorHeap>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mHeaps;
		std::array<UINT, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mOffsets;
		WRL::ComPtr<ID3D12Device>			mDevice;
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