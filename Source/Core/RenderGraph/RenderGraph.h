#pragma once
#include<variant>
#include<vector>
#include"RenderGraphUtils.h"
#include"RenderGraphPass.h"

struct RenderGraphNode
{
	RenderPassID id;
	std::vector<RenderPassID> outDegree;
	int inDegreeSize = 0;
};

class GraphCompiler;
class RenderGraph
{
	friend class GraphCompiler;
public:
	// Singleton
	static	RenderGraph* GetInstance()
	{
		static RenderGraph gRenderGraph;
		return &gRenderGraph;
	}

	template<typename... NodeArgs>
	RenderPassID CreateGraphNode(NodeArgs&&... args)
	{
		auto id = mNodeGraphs.size();
		RenderGraphPass* node = new RenderGraphPass(std::forward<NodeArgs>(args));
		node->mRootGraph = this;
		mGraphNodes.push_back(node);
		mPassMap[id] = node;
		return static_cast<RenderPassID>(id);
	}



	RenderGraphPass* GetPass(RenderPassID passID)
	{
		auto p = mPassMap.find(passID);
		if (p != mPassMap.end())
			return p->second;
		else
			return nullptr;
	}


protected:
	std::vector<RenderGraphPass*> mGraphNodes;
	std::unordered_map<RenderPassID, RenderGraphPass*> mPassMap;
};