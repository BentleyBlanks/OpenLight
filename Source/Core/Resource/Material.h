#pragma once
#include<DirectXMath.h>

using namespace DirectX;
namespace OpenLight
{
	struct Material
	{
		Material() {}
		Material(const Material& rhs)
		{
			albedo = rhs.albedo;
			roughness = rhs.roughness;
			metallic = rhs.metallic;
		}
		Material(const XMFLOAT4& albedo, const XMFLOAT4& roughness, const XMFLOAT4& metallic)
		{
			this->albedo = albedo;
			this->roughness = roughness;
			this->metallic = metallic;
		}
		Material& operator=(const Material& rhs)
		{
			albedo = rhs.albedo;
			roughness = rhs.roughness;
			metallic = rhs.metallic;
		}
		

		XMFLOAT4 albedo;
		XMFLOAT4 roughness;
		XMFLOAT4 metallic;
	};
}
