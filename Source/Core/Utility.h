#pragma once

#include <Windows.h>
#include <algorithm>
#include <DirectXMath.h>

#define D3D_HLSL_DEFUALT_INCLUDE ((ID3DInclude*)(UINT_PTR)1)
#define PAD(x,y)		((x+ (y-1) )&(~(y-1)))
#define PADCB(x)		PAD(x,256)
using namespace DirectX;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))	throw std::exception();
}

inline XMFLOAT3 normalize(const XMFLOAT3& v)
{
	auto  l = std::sqrtf(v.x * v.x + v.y * v.y + v.z *v.z);
	return XMFLOAT3(v.x / l, v.y / l, v.z / l);
}

inline XMFLOAT3 cross(const XMFLOAT3& v1, const XMFLOAT3& v2)
{
	return XMFLOAT3(v1.y*v2.z - v1.z*v2.y,
		v1.z*v2.x - v1.x*v2.z,
		v1.x*v2.y - v1.y*v2.x);
}

inline XMFLOAT3 operator+(const XMFLOAT3& v1, const XMFLOAT3& v2)
{
	return XMFLOAT3(v1.x + v2.x,
		v1.y + v2.y,
		v1.z + v1.z);
}

inline XMFLOAT3 operator-(const XMFLOAT3& v1, const XMFLOAT3& v2)
{
	return XMFLOAT3(v1.x - v2.x,
		v1.y - v2.y,
		v1.z - v2.z);
}

inline XMFLOAT2 operator+(const XMFLOAT2& v1, const XMFLOAT2& v2)
{
	return XMFLOAT2(v1.x + v2.x,
		v1.y + v2.y);
}

inline XMFLOAT2 operator-(const XMFLOAT2& v1, const XMFLOAT2& v2)
{
	return XMFLOAT2(v1.x - v2.x,
		v1.y - v2.y);
}