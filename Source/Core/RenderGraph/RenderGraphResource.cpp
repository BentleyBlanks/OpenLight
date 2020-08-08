#include "RenderGraphResource.h"
#include "App.h"
#include "Renderer.h"
#include "Utility.h"
LogicalResourceID GraphResourceMgr::CreateLogicalResource(const std::string& name)
{
	LogicalResourceID id = LogicalResourceID(mLogicalResources.size());
	LogicalResource resource;
	resource.name = name;
	resource.type = ELogical_Unknown;

	mLogicalResources[id] = resource;

	return id;
}

LogicalResourceID GraphResourceMgr::CreateLogicalResource(const std::string& name, const PhysicalDesc& desc)
{
	LogicalResourceID id = LogicalResourceID(mLogicalResources.size());

	LogicalResource resource;
	resource.name = name;
	resource.physicalDesc = desc;
	resource.type = ELogical_Implicit;
	mLogicalResources[id] = resource;
	return id;
}

LogicalResourceID GraphResourceMgr::CreateLogicalResource(const std::string& name, const PhysicalResourceID& physicalID)
{
	LogicalResourceID id = LogicalResourceID(mLogicalResources.size());
	LogicalResource resource;
	resource.name = name;
	resource.physicalDesc = mPhysicalResources[physicalID].desc;
	resource.type = ELogical_Explicit;
	return id;
}

void GraphResourceMgr::BindUnknownResource(const LogicalResourceID& logicalID, const PhysicalResourceID& physicalID)
{

	auto& logical = mLogicalResources[logicalID];
	_ASSERT(logical.type == ELogical_Unknown);
	logical.physicalDesc = mPhysicalResources[physicalID].desc;
	logical.type = ELogical_Implicit;

}


PhysicalResourceID GraphResourceMgr::LoadPhysicalResource(const PhysicalResource& physical)
{
	PhysicalResourceID id = PhysicalResourceID(mPhysicalResources.size());
	mPhysicalResources[id] = physical;
	mPhysicalResources[id].type = EPhysical_Explicit;
	return id;
}

void GraphResourceMgr::BeginFrame()
{
	mActivatedPhysicalResources.clear();
	mUnusedPhysicalResources.clear();
	for (const auto& physicalResourcePair : mPhysicalResources)
	{
		assert(physicalResourcePair.second.type != EPhysical_Unknown);

		if (physicalResourcePair.second.type == EPhysical_Explicit)
			mActivatedPhysicalResources.push_back(physicalResourcePair.first);
		else
			mUnusedPhysicalResources.push_back(physicalResourcePair.first);
	}
}

void GraphResourceMgr::EndFrame()
{
}




PhysicalResourceID GraphResourceMgr::ConstructPhysicalResource(const PhysicalDesc& desc)
{
	for (auto p = mUnusedPhysicalResources.begin(); p != mUnusedPhysicalResources.end(); ++p)
	{
		auto& physicalResource = mPhysicalResources[*p];

		if (bitwise_equal(desc, physicalResource.desc))
		{
			auto id = (*p);
			mUnusedPhysicalResources.erase(p);
			return id;
		}
	}
	
	// There is not physical resource that could match given desc
	// Create it !
	return CreatePhysicalResource(desc);
}
void GraphResourceMgr::DestroyPhysicalResource(const PhysicalResourceID& physicalResourceID)
{
	for (auto p = mActivatedPhysicalResources.begin(); p != mActivatedPhysicalResources.end(); ++p)
	{
		if ((*p) == physicalResourceID)
		{

			mActivatedPhysicalResources.erase(p);
			mUnusedPhysicalResources.push_back(physicalResourceID);
			return;
		}
	}
	assert(false);
}
PhysicalResourceID GraphResourceMgr::CreatePhysicalResource(const PhysicalDesc& desc)
{
	auto device = App::GetInstance().GetRenderer()->GetDevice();
	auto id = PhysicalResourceID(mPhysicalResources.size());
	PhysicalResource physicalResource;
	physicalResource.desc      = desc;
	physicalResource.initState = D3D12_RESOURCE_STATE_COMMON;
	physicalResource.state     = D3D12_RESOURCE_STATE_COMMON;
	physicalResource.type      = EPhysical_Implicit;
	
	
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc.desc,
		D3D12_RESOURCE_STATE_COMMON,
		&desc.initialiValue,
		IID_PPV_ARGS(&physicalResource.resource)));

	mPhysicalResources[id] = physicalResource;
	return id;
}



