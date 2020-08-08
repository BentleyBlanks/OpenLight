#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include<unordered_map>
#include "RenderGraphUtils.h"


/*
	Physical Resource Description
	bound to logical resource for searching a real physical resource
*/
struct PhysicalDesc
{
	CD3DX12_RESOURCE_DESC desc;
	D3D12_CLEAR_VALUE	  initialiValue;
};


enum ELogicalType
{
	ELogical_Explicit = 0,
	ELogical_Implicit = 1,
	ELogical_Unknown  = 2
};
/*
	Logical Resource
	just used to construct render graph
*/
class LogicalResource
{
public:
	std::string			name;
	size_t				resource;
	PhysicalDesc		physicalDesc;

	// implicit or explicit

	// implicit: Bind a matched physical resource created or searched automaticlly to this logical resource
	// explicit: Bind a given physical resource to this logical resource
	ELogicalType		 type;


	PhysicalResourceID	physicalResourceID;
};

enum EPhysicalType
{
	EPhysical_Explicit = 0,
	EPhysical_Implicit = 1,
	EPhysical_Unknown = 2
};
/*
	Physical Resource
	the real resource allocated on GPU

	Note: reused in every frame
*/
class PhysicalResource
{
public:
	ID3D12Resource*					resource = nullptr;
	D3D12_RESOURCE_STATES			initState;
	volatile D3D12_RESOURCE_STATES	state;
	PhysicalDesc					desc;

	// Explicit: a kind of physical resources given by program
	// Implicit: a kind of logical resource created by GraphResourceMgr
	EPhysicalType					type;
};

/*
	Descriptor Resource
	describe how to use an array of logical resource in render pass.
	means that a descriptor resource could be bound to several logical resource
	
	Note: may allocated in every frame
*/
class DescriptorResource
{
public:

	CD3DX12_GPU_DESCRIPTOR_HANDLE	gpuHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	cpuHandle;
	bool							isInit = false;

};

struct DescriptorDesc
{
	DescriptorDesc(
		const std::vector<LogicalResourceID>& resources,
		const std::vector<D3DViewDesc>& descs,
		const std::vector<D3D12_RESOURCE_STATES>& rstates
	) :boundLogicalResources(resources), viewDescs(descs), states(rstates) {}
	std::vector<LogicalResourceID>			boundLogicalResources;
	std::vector<D3DViewDesc>				viewDescs;
	std::vector<D3D12_RESOURCE_STATES>		states;
	D3D12_DESCRIPTOR_HEAP_TYPE				heapType;
};

class GraphExecutor;
class GraphCompiler;
class RenderGraphPass;
// 
class GraphResourceMgr
{
	friend GraphExecutor;
public:
	// Singleton 
	static GraphResourceMgr* GetInstance()
	{
		static GraphResourceMgr mgr;
		return &mgr;
	}

	// Create Logical Resource
	LogicalResourceID CreateLogicalResource(const std::string& name);
	LogicalResourceID CreateLogicalResource(const std::string& name, const PhysicalDesc& desc);
	LogicalResourceID CreateLogicalResource(const std::string& name, const PhysicalResourceID& physicalID);
	void	BindUnknownResource(const LogicalResourceID& logicalID, const PhysicalResourceID& physicalID);


	// Load existed physical resource
	PhysicalResourceID LoadPhysicalResource(const PhysicalResource& physical);

	// Begin Frame
	void BeginFrame();
	
	// End Frame
	void EndFrame();

	// Constrcut a matched physical resource for given physical desc
	PhysicalResourceID ConstructPhysicalResource(const PhysicalDesc& desc);

	// Destroy a physical resource
	void DestroyPhysicalResource(const PhysicalResourceID& physicalResourceID);

	LogicalResource* GetLogicalResource(const LogicalResourceID& id)
	{ 
		auto p = mLogicalResources.find(id);
		if (p == mLogicalResources.end())
			return nullptr;
		else
			return &p->second;
	}
	PhysicalResource* GetPhysicalResource(const PhysicalResourceID& id) 
	{
		auto p = mPhysicalResources.find(id);
		if (p == mPhysicalResources.end())
			return nullptr;
		else
			return &p->second;
	}





protected:
	// Create Physical Resource
	PhysicalResourceID CreatePhysicalResource(const PhysicalDesc& desc);
	
	// All of logical resources
	std::unordered_map<LogicalResourceID, LogicalResource>		mLogicalResources;
	// All of physical resources
	std::unordered_map<PhysicalResourceID, PhysicalResource>	mPhysicalResources;
	

	// activated physical resource
	std::list<PhysicalResourceID>								mActivatedPhysicalResources;

	// Unused physical resource
	std::list<PhysicalResourceID>								mUnusedPhysicalResources;

	// Logical <-> Physical
//	std::unordered_map<LogicalResource, PhysicalResourceID>		mLogical2Physical;




};