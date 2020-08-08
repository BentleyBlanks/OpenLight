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



	RenderPassID CreateRenderPass(const std::string& passName)
	{
		auto id = mGraphNodes.size();
		RenderGraphPass* pass = new RenderGraphPass(passName, id);
		mGraphNodes.push_back(pass);
		pass->mRootGraph = this;
		mPassMap[id] = pass;
		return RenderPassID(id);
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