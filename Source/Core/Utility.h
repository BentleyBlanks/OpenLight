#pragma once

#include <Windows.h>
#include <algorithm>
#include <DirectXMath.h>
#include <sstream>
#include <fstream>
#include <cstddef>
#include <vector>
#define D3D_HLSL_DEFUALT_INCLUDE ((ID3DInclude*)(UINT_PTR)1)
#define PAD(x,y)		((x+ (y-1) )&(~(y-1)))
#define PADCB(x)		PAD(x,256)
#define ReleaseCom(p)	{ if(p) {p->Release();p=nullptr;}}
#define ErrorBox(text) {MessageBox(0, text, 0, 0);}
#define ErrorBoxW(text) {MessageBoxW(0, text, 0, 0);}
using namespace DirectX;

enum EFresnelMaterialName
{
	EWater = 0,
	EPlasticLow,
	EPlasticHigh,
	EGlass,
	EDiamond,
	EIron,
	ECopper,
	EGold,
	EAluminum,
	ESilver,
	EFresnelMaterialNameCount
};



inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		std::ostringstream oss;
		oss << std::hex << hr;
		ErrorBox(oss.str().c_str());
		throw std::exception();
	}
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
		v1.z + v2.z);
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

class RawMemoryParser
{
public:
	static std::vector<BYTE> readData(const std::string& file)
	{
		std::ifstream fin;
		fin.open(file, std::ios_base::binary);
		assert(!fin.fail());
		fin.seekg(0, std::ios::end);
		std::vector<BYTE> data(fin.tellg());
		fin.seekg(std::ios::beg);
		fin.read(reinterpret_cast<char*>(&data[0]), data.size());
		fin.close();
		return data;
	}
};
