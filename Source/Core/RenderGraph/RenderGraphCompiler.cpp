#include<algorithm>
#include "RenderGraphCompiler.h"
#include "Resource/GPUResource.h"
#include "Renderer.h"
#include<queue>
GraphCompiledResult GraphCompiler::Compile(RenderGraph& graph) const
{
	GraphCompiledResult compiledResult;
	const auto& passes = graph.mGraphNodes;

	for (const auto& pass : passes)
	{
		for (const LogicalResourceID& inputID : pass->mInputResources)
			compiledResult.logicalResourceCompiledInfos[inputID].readers.push_back(pass->mPassID);
		for (const LogicalResourceID& outputID : pass->mOutputResources)
			compiledResult.logicalResourceCompiledInfos[outputID].writer = pass->mPassID;
	}
	auto& dag = compiledResult.dag;
	for (auto& p : compiledResult.logicalResourceCompiledInfos)
	{
		// TODO: valid assert
//		_ASSERT(true);


		auto& node = compiledResult.dag.nodes[p.second.writer];
		for (const auto& reader : p.second.readers)
		{
			node.outDegree.push_back(reader);
			dag.nodes[reader].inDegreeSize++;
		}
	}

	compiledResult.sortedPass = TopoSort(dag);

	// RenderPassID <-> execute order
	std::unordered_map<RenderPassID, size_t> orders;
	for (size_t i = 0; i < compiledResult.sortedPass.size(); ++i)
	{
		orders[compiledResult.sortedPass[i]] = i;
	}

	auto& passInfo = compiledResult.renderPassCompiledInfos;
	for (auto& resInfo : compiledResult.logicalResourceCompiledInfos)
	{
		//TODO: write valid?
		auto& info = resInfo.second;
		info.start = orders.size();
		info.end = 0;
		if (IsValidID(info.writer))
		{
			info.start = orders[info.writer];
			info.end = orders[info.writer];
		}
		for (auto& reader : info.readers)
		{
			info.start = std::min(info.start, orders[reader]);
			info.end = std::max(info.end, orders[reader]);
		}

		passInfo[compiledResult.sortedPass[info.start]].needToConstruct.push_back(resInfo.first);
		passInfo[compiledResult.sortedPass[info.end]].needToDestroy.push_back(resInfo.first);

	}
	compiledResult.renderGraph = &graph;

	return compiledResult;
}

std::vector<RenderPassID> GraphCompiler::TopoSort(GraphCompiledResult::DAG dag) const
{
	std::queue<RenderPassID > Q;
	for (auto& nodePair : dag.nodes)
	{
		if (nodePair.second.inDegreeSize == 0)
		{
			Q.push(nodePair.first);
		}
	}
	std::vector<RenderPassID> orderedPass;
	while (!Q.empty())
	{
		auto pass = Q.front();
		Q.pop();
		orderedPass.push_back(pass);

		for (auto& adj : dag.nodes[pass].outDegree)
		{
			auto& adjNode = dag.nodes[adj];
			adjNode.inDegreeSize--;

			if (adjNode.inDegreeSize == 0)
				Q.push(adj);
		}

	}
	return orderedPass;
}

//// TODO MACRO
//#define Allocate_DescriptorResource
//#define Allocate_PassResource
//#define Construct_Resource
//#define Destroy_Resource

void GraphExecutor::Execute(GraphCompiledResult& compiledResult)
{
	// Allocator Descriptor Heap address
	// just allocate and not construct object
	auto dynamicHeap = OpenLight::GPUDynamicDescriptorHeapWrap::GetInstance();
	dynamicHeap->Reset();
	for (auto& passID : compiledResult.sortedPass)
	{
		auto pass = compiledResult.renderGraph->GetPass(passID);
		
		for (size_t i = 0; i < pass->mDescriptorDescs.size(); ++i)
		{
			auto& descriptorResource  = pass->mDescriptorResources[i];
			auto& desc                = pass->mDescriptorDescs[i];
			descriptorResource.isInit = false;
			if (desc.heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
			{
				auto handle = dynamicHeap->AllocateGPU(desc.boundLogicalResources.size());
				descriptorResource.gpuHandle = handle.first;
				descriptorResource.cpuHandle = handle.second;
			}
			else if (desc.heapType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
			{
				auto handle = dynamicHeap->AllocateRTV(desc.boundLogicalResources.size());
				descriptorResource.cpuHandle = handle;
			}
			else
			{
				auto handle = dynamicHeap->AllocateDSV(desc.boundLogicalResources.size());
				descriptorResource.cpuHandle = handle;
			}
		}
	}
	
	// Initalize CmdList and CmdAllocator
	auto cmdList = MacroGetCmdList();
	auto cmdAllocator = MacroGetCurrentCmdAllocator();
	//cmdAllocator->Reset();
	//cmdList->Reset(cmdAllocator, nullptr);
	for (auto& passID : compiledResult.sortedPass)
	{
		auto pass = compiledResult.renderGraph->GetPass(passID);
		ConstructResource(passID, compiledResult);
		pass->Execute();
		DestroyResource(passID,compiledResult);
	}


}

void GraphExecutor::ConstructResource(const RenderPassID& renderPassID, GraphCompiledResult& compiledInfo)
{
	const auto& passCompiledInfo = compiledInfo.renderPassCompiledInfos[renderPassID];
	auto resourceMgr             = GraphResourceMgr::GetInstance();
	auto pass                    = compiledInfo.renderGraph->GetPass(renderPassID);
	auto device                  = MacroGetDevice();
	auto cmdList				 = MacroGetCmdList();


	// Step 1: Allocate physical resource
	for (auto& logicalResourceID : passCompiledInfo.needToConstruct)
	{
		// Get Logical Resource
		auto logicalResource = resourceMgr->GetLogicalResource(logicalResourceID);
		assert(logicalResource->type != ELogical_Unknown);

		if (logicalResource->type == ELogical_Explicit)
		{
			assert(IsValidID(logicalResource->physicalResourceID));
		}
		else
		{
			logicalResource->physicalResourceID = resourceMgr->ConstructPhysicalResource(logicalResource->physicalDesc);
			assert(IsValidID(logicalResource->physicalResourceID));
		}
		
		
	}

	// Step 2:Allocate d3d12 descriptor view
	std::vector<D3D12_RESOURCE_BARRIER> resourceBarriers;
	for (size_t i = 0; i < pass->mDescriptorDescs.size(); ++i)
	{
		const auto& descriptorDesc = pass->mDescriptorDescs[i];
		auto& descriptor = pass->mDescriptorResources[i];
		// Create Descriptor
		auto cpuHandle = descriptor.cpuHandle;
		for (size_t resIdx = 0; resIdx < descriptorDesc.boundLogicalResources.size(); ++resIdx)
		{
			
			auto& d3dDesc         = descriptorDesc.viewDescs[resIdx];
			auto logicalResource  = resourceMgr->GetLogicalResource(descriptorDesc.boundLogicalResources[resIdx]);
			auto physicalResource = resourceMgr->GetPhysicalResource(logicalResource->physicalResourceID);

			switch (d3dDesc.index())
			{
			case 0:
			{
				// D3D12_CONSTANT_BUFFER_VIEW_DESC
				auto cbvDesc = std::get<0>(d3dDesc);
				cbvDesc.BufferLocation = physicalResource->resource->GetGPUVirtualAddress();
				device->CreateConstantBufferView(&cbvDesc, cpuHandle);
				cpuHandle.Offset(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
				break;
			}
			case 1:
			{
				// D3D12_SHADER_RESOURCE_VIEW_DESC
				auto& srvDesc = std::get<1>(d3dDesc);
				device->CreateShaderResourceView(physicalResource->resource,
					&srvDesc, cpuHandle);
				cpuHandle.Offset(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
				break;
			}
			case 2:
			{
				// D3D12_UNORDERED_ACCESS_VIEW_DESC
				auto& uavDesc = std::get<2>(d3dDesc);
				device->CreateUnorderedAccessView(physicalResource->resource, nullptr,
					&uavDesc, cpuHandle);
				cpuHandle.Offset(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
				break;
			}
			case 3:
			{
				// D3D12_RENDER_TARGET_VIEW_DESC
				auto& rtvDesc = std::get<3>(d3dDesc);
				device->CreateRenderTargetView(physicalResource->resource,
					&rtvDesc, cpuHandle);
				cpuHandle.Offset(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
				break;
			}
			case 4:
			{
				// D3D12_DEPTH_STENCIL_VIEW_DESC
				auto& dsvDesc = std::get<4>(d3dDesc);
				device->CreateDepthStencilView(physicalResource->resource,
					&dsvDesc, cpuHandle);
				cpuHandle.Offset(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
				break;
			}
			default:
				assert(false);
				break;
			}

			// if necessary, switch physical resource state
			if (physicalResource->state != descriptorDesc.states[resIdx])
			{
				D3D12_RESOURCE_BARRIER barrier;
				barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrier.Transition.pResource   = physicalResource->resource;
				barrier.Transition.StateBefore = physicalResource->state;
				barrier.Transition.StateAfter  = descriptorDesc.states[resIdx];
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				resourceBarriers.push_back(barrier);
				physicalResource->state = descriptorDesc.states[resIdx];
			}
		}

		descriptor.isInit = true;
	}

	if (!resourceBarriers.empty())
	{
		cmdList->ResourceBarrier(resourceBarriers.size(), &resourceBarriers[0]);
	}
}

void GraphExecutor::DestroyResource(const RenderPassID& renderPassID, GraphCompiledResult& compiledInfo)
{
	const auto& passCompiledInfo = compiledInfo.renderPassCompiledInfos[renderPassID];
	auto resourceMgr = GraphResourceMgr::GetInstance();
	auto pass = compiledInfo.renderGraph->GetPass(renderPassID);
	auto device = MacroGetDevice();

	for (auto& logicalResourceID : passCompiledInfo.needToDestroy)
	{
		
		auto logicalResource = resourceMgr->GetLogicalResource(logicalResourceID);
		if (logicalResource->type != ELogical_Explicit)
			resourceMgr->DestroyPhysicalResource(logicalResource->physicalResourceID);
	}
}
