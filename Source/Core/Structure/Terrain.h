#pragma once
#include"Utility.h"
#include<string>
#include<vector>
#include"ObjMeshLoader.h"
#include "Structure/CBStruct.h"
#include"Resource/GPUResource.h"
#include "Structure/AABB.h"
using namespace OpenLight;
class Terrain
{
public:
	Terrain(
		ID3D12Device5* device,
		ID3D12GraphicsCommandList* commandList,
		ID3D12CommandQueue* commandQueue,
		GPUDescriptorHeapWrap* descriptorHeap,
		const std::string& fileName, float scale, float widthScale = 1.f, float heightScale = 1.f);
	~Terrain();
	
	UINT getVertexCol()		const { return mWidth; }
	UINT getVertexRow()		const { return mHeight; }
	UINT getGridCol()		const { return mWidth - 1; }
	UINT getGridRow()		const { return mHeight - 1; }

	

	// 得到给定位置的顶点信息
	StandardVertex&			operator()(UINT row, UINT col)		;
	const StandardVertex&	operator()(UINT row, UINT col)	const;

	// 得到某个位置的高度
	float					getHeight(float x, float y) const;

	ID3D12Resource*				vb     = nullptr;
	ID3D12Resource*				ib     = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	vbView = {};
	D3D12_INDEX_BUFFER_VIEW		ibView = {};
	XMFLOAT4X4					world;

	DescriptorIndex	diffuseSRVIndex    = {};
	ID3D12Resource* diffuseMap         = nullptr;
	
	AABB			aabb;

	std::vector<StandardVertex> mVertices;
	std::vector<UINT>			mIndices;
protected:
	void loadHeightMap(const std::string& fileName, float scale);
	void constructTerrain();

	UINT calIndex(UINT i, UINT j) const { return j * mWidth + i; }
	XMFLOAT3 calNormal(const XMFLOAT3& p0, const XMFLOAT3& p1, const XMFLOAT3& p2) const;

	std::vector<float>			mTerrainHeight;

	float						mScale;
	float						mWidthScale;
	float						mHeightScale;
	UINT						mWidth;
	UINT						mHeight;
	float						mHalfWidth;
	float						mHalfHeight;

	ID3D12Device5*				mDevice = nullptr;
	

};


class Grass
{
public:
	Grass(ID3D12Device5* device,
		ID3D12CommandAllocator* commandAllocator,
		ID3D12GraphicsCommandList* commandList,
		ID3D12CommandQueue* commandQueue,
		GPUDescriptorHeapWrap* descriptorHeap,
		Terrain* terrain = nullptr);
	~Grass()
	{
		ReleaseCom(diffuseMap);
		ReleaseCom(alphaMap);
		ReleaseCom(vb);
	}
	ID3D12Resource* vb              = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	CBGrassInfo		grassInfo;

	ID3D12Resource* diffuseMap      = nullptr;
	ID3D12Resource* alphaMap        = nullptr;
	DescriptorIndex diffuseAlphaIndex = {};
	auto getGrassCount() const { return mGrassRoots.size(); }
private:

	std::vector<StandardVertex>	mGrassRoots;
	
	ID3D12Device5*	mDevice          = nullptr;

};