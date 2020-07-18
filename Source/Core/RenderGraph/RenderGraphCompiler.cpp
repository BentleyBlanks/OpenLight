#include<algorithm>
#include "RenderGraphCompiler.h"
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
		_ASSERT(true);


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
		if (!info.writer.IsValid())
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

// TODO MACRO
#define Allocate_DescriptorResource
#define Allocate_PassResource
#define Construct_Resource
#define Destroy_Resource

void GraphExecutor::Execute(const GraphCompiledResult& compiledResult)
{
	Allocate_DescriptorResource(compiledResult);

	for (auto& passID : compiledResult.sortedPass)
	{
		auto pass = compiledResult.renderGraph->GetPass(passID);
		Construct_Resource(pass);
		pass->Execute();
		Destroy_Resource(pass);
	}


}
