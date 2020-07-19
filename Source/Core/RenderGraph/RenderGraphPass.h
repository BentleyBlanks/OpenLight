#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include<vector>
#include<unordered_map>
#include<functional>
#include "d3dx12.h"
#include "RenderGraphUtils.h"

#include "RenderGraphResource.h"
class RenderGraph;




class GraphCompiler;
class GraphExecutor;
class RenderGraphPass
{
	friend RenderGraph;
	friend GraphCompiler;
	friend GraphExecutor;
public:
	RenderGraphPass(const RenderPassID& passID) { mPassID = passID; }
	RenderGraphPass(const RenderPassID& passID, const std::function<void(const RenderGraphPass*)>& setupFunc, const std::function<void(const RenderGraphPass*)>& executeFunc)
	{
		mPassID = passID;
		mSetupFunc = setupFunc;
		mExecuteFunc = executeFunc;
	}
	RenderGraphPass(
		const RenderPassID& passID,
		const std::function<void(const RenderGraphPass*)>& setupFunc,
		const std::function<void(const RenderGraphPass*)>& executeFunc,
		const std::vector<LogicalResourceID>& InputResources,
		const std::vector<LogicalResourceID>& OutputResources
	) :RenderGraphPass(passID, setupFunc, executeFunc)
	{
		BindInput(InputResources);
		BindOutput(OutputResources);
	}

	virtual void Setup()
	{
		mSetupFunc(this);
	}

	virtual void Execute()
	{
		mExecuteFunc(this);
	}

	virtual void BindInput(const std::vector<LogicalResourceID>& InputResources)
	{
		// Rebind is forbidden for the moment
		assert(mInputResources.empty());
		mInputResources = InputResources;
	}

	virtual void BindOutput(const std::vector<LogicalResourceID>& OutputResources)
	{
		// Rebind is forbidden for the moment
		assert(mOutputResources.empty());
		mOutputResources = OutputResources;
	}

	virtual void BindLogicalDescriptor(const std::vector<LogicalResourceID>& logicalResources, const DescriptorDesc& desc)
	{

		/*
			Descritpor Reousrce is a kind of light resource which can be created at each frame
			So the process of allocation will be delayed until EXECUTE starts
		*/
		DescriptorResourceID descriptorID = DescriptorResourceID(mDescriptorDescs.size());

		/*
			just need to store desc
		*/
		mDescriptorDescs.push_back(desc);
		mDescriptorResources.push_back(DescriptorResource());
		for (const auto& logicalID : logicalResources)
		{
			mLogical2Descriptor[logicalID] = descriptorID;
		}

	}





protected:

	// Callable Object
	std::function<void(const RenderGraphPass*)>			mSetupFunc;
	std::function<void(const RenderGraphPass*)>			mExecuteFunc;



	// Input Resources
	std::vector<LogicalResourceID>		mInputResources;
	// Output Resources
	std::vector<LogicalResourceID>		mOutputResources;
	// Logical <-> Descriptor Resource
	std::unordered_map<LogicalResourceID, DescriptorResourceID> mLogical2Descriptor;
	// Descriptor Resource
	std::vector<DescriptorDesc>			mDescriptorDescs;
	std::vector<DescriptorResource>		mDescriptorResources;

	RenderPassID						mPassID;
	std::string							mPassName;
	const RenderGraph* mRootGraph = nullptr;
};