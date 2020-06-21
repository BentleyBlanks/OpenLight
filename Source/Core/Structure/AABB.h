#pragma once
#include<DirectXMath.h>
#include<limits>
using namespace DirectX;

struct AABB
{
	AABB()
	{
		minP = XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		maxP = XMFLOAT3(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	}

	void ExtendAABB(XMFLOAT3 p)
	{
		XMVECTOR minV, maxV;
		minV = XMLoadFloat3(&minP);
		maxV = XMLoadFloat3(&maxP);
		XMVECTOR vp = XMLoadFloat3(&p);
		maxV = XMVectorMax(vp, maxV);
		minV = XMVectorMin(vp, minV);
		XMStoreFloat3(&minP, minV);
		XMStoreFloat3(&maxP, maxV);

	}

	float Extent(int i)
	{
		if (i == 0)
			return maxP.x - minP.x;
		else if (i == 1)
			return maxP.y - minP.y;
		else
			return maxP.z - minP.z;
	}

	XMFLOAT3 minP;
	XMFLOAT3 maxP;
};