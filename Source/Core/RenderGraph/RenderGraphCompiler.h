#pragma once
#include "RenderGraph.h"
struct GraphCompiledResult
{
	// Logical Resource information after being compiled
	struct LogicalResourceCompiledInfo
	{
		// lifetime
		// [start,end]
		size_t start;
		size_t end;


		std::vector<RenderPassID> readers;
		RenderPassID				writer;
	};

	// Render Pass infomation after being compiled
	struct RenderPassCompiledInfo
	{
		// Resources to construct before EXECUTE
		std::vector<LogicalResourceID>	needToConstruct;

		// Resources to desctroy after EXECUTE
		std::vector<LogicalResourceID>	needToDestroy;
	};

	// Directed Acyclic Graph
	struct DAG
	{
		std::unordered_map<RenderPassID, RenderGraphNode> nodes;
	}dag;

	std::unordered_map<LogicalResourceID, LogicalResourceCompiledInfo>	logicalResourceCompiledInfos;
	std::unordered_map<RenderPassID, RenderPassCompiledInfo>					renderPassCompiledInfos;

	// Toposort Resource
	std::vector<RenderPassID>													sortedPass;

	RenderGraph* renderGraph;
};
class GraphCompiler
{
public:
	// Singleton
	static GraphCompiler* GetInstance()
	{
		static GraphCompiler compiler;
		return &compiler;
	}



	GraphCompiledResult Compile(RenderGraph& graph) const;

protected:
	std::vector<RenderPassID>	TopoSort(GraphCompiledResult::DAG dag) const;


};


class GraphExecutor
{
public:
	// Singleton
	static GraphExecutor* GetInstance()
	{
		static GraphExecutor exe;
		return &exe;
	}

	virtual void Execute(const GraphCompiledResult& compiledResult);
};