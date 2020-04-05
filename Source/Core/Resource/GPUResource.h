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
	class GPUDescriptorRegister;

	enum EGPUDescriptorTableType
	{
		EGPUDescriptorTableType_Comman,
		EGPUDescriptorTableType_Sampler,
		EGPUDescriptorTableType_NONE
	};
	class GPUDescriptorTable
	{

	public:
		friend GPUDescriptorRegister;
		
		GPUDescriptorTable() {  size = 0; }
		D3D12_GPU_DESCRIPTOR_HANDLE castHande()
		{
			assert(hasRegister && size > 0);

			return offset;
			
		}
	protected:
		CD3DX12_GPU_DESCRIPTOR_HANDLE offset;
		UINT size;
		bool hasRegister = false;
		GPUDescriptorRegister* gpuDescriptorRegister = nullptr;
	};

	// 需要使用 GPU 资源的类
	class GPUDescriptorRegister
	{
	public:
		GPUDescriptorRegister(WRL::ComPtr<ID3D12Device> device);

		void RegisterSRV(const std::string& resourceName)
		{
			RegisterSRVCBVUAV(resourceName);
		}
		void RegisterCBV(const std::string& resourceName)
		{
			RegisterSRVCBVUAV(resourceName);
		}
		void RegisterUAV(const std::string& resourceName)
		{
			RegisterSRVCBVUAV(resourceName);
		}

		void RegisterSRVCBVUAV(const std::string& resourceName)
		{
			auto& hash = mResourceHashs[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
			if (hash.find(resourceName) == hash.end())
				hash.insert(resourceName);
			else
			{
				std::cout << "Multiple Register Same Resource: " << resourceName << ": as SRVCBVUAV!" << std::endl;
			}
		}
		void RegisterSampler(const std::string& resourceName)
		{
			auto& hash = mResourceHashs[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER];
			if (hash.find(resourceName) == hash.end())
				hash.insert(resourceName);
			else
			{
				std::cout << "Multiple Register Same Resource: " << resourceName << ": as Sampler!" << std::endl;
			}
		}

		


		void RegisterDescriptor(const std::string& tableName, GPUDescriptorTable* table, EGPUDescriptorTableType type);

		void AddSRV(const std::string& resourceName,
			WRL::ComPtr<ID3D12Resource> resource,
			const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);

		void AddCBV(const std::string& resourceName,
			WRL::ComPtr<ID3D12Resource> resource,
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc);

		void AddUAV(const std::string& resourceName,
			WRL::ComPtr<ID3D12Resource> resource,
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc,
			WRL::ComPtr<ID3D12Resource> resourceCount = nullptr);

		void AddSampler(const std::string& resourceName,
			D3D12_SAMPLER_DESC samplerDesc);

		void CommitRegisterDescriptorTable();

		ID3D12DescriptorHeap* CommonHeap()	{ return mCommonHeap.Get(); }
		ID3D12DescriptorHeap* SamplerHeap() { return mSamplerHeap.Get(); }
	protected:
		// SRV CBV UAV
		WRL::ComPtr<ID3D12DescriptorHeap>	mCommonHeap = nullptr;
		UINT								mCommonOffset = 0;
		// Sampler
		WRL::ComPtr<ID3D12DescriptorHeap>	mSamplerHeap = nullptr;
		UINT								mSamplerOffset = 0;
		
		std::array<std::unordered_set<std::string>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mResourceHashs;
		std::unordered_set<std::string>		mDescriptorTableHash;
		WRL::ComPtr<ID3D12Device>			mDevice;

		bool								mRunning = false;
		GPUDescriptorTable*					mRunningTable = nullptr;
		EGPUDescriptorTableType				mRunningTableType;

	};



	
}

#define RESOURCE_IID_ARGS(Resource)  #Resource,Resource
#define RESOURCE_IID(Resource)	#Resource
#define RESOURCE_IID_PARGS(Resource) #Resource,&Resource
