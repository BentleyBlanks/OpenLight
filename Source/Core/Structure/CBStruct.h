#pragma once
#include "Utility.h"
#include <d3d12.h>
#include "d3dx12.h"
#include "Resource/GPUResource.h"
using DescriptorIndex = OpenLight::GPUDescriptorHeapWrap::GPUDescriptorHeapWrapIndex;
struct CBMaterial
{
	// rgb: albedo
	//   a: alpha
	XMFLOAT4 materialAlbedo;
	// r: roughness 
	// g: metallic
	// b: ao 
	XMFLOAT4 materialInfo;
};

struct MaterialTexture
{
	ID3D12Resource* albedoMap    = nullptr;
	DescriptorIndex albedoIndex;
	ID3D12Resource* normalMap    = nullptr;
	DescriptorIndex normalIndex;
	ID3D12Resource* metallicMap  = nullptr;
	DescriptorIndex metallicIndex;
	ID3D12Resource* roughnessMap = nullptr;
	DescriptorIndex roughnessIndex;
};


struct CBTrans
{
	
	XMFLOAT4X4 wvp;
	XMFLOAT4X4 world;
	XMFLOAT4X4 invTranspose;

};

struct CBCamera
{
	XMFLOAT4 cameraPositionW;
};

struct CBPointLights
{
	XMFLOAT4 lightPositions[4];
	XMFLOAT4 lightIntensitys[4];
	XMFLOAT4 lightSizeFloat;
};

struct CBIBLInfo
{
	// r: wi 半球上的 Theta 采样数 [0,PI/2]
	// g: wi 半球上的 Phi   采样数 [0,2PI]
	// b: 给定半球上的 Theta 步幅   
	// a: 给定半球上的 Phi   步幅
	XMFLOAT4 IBLDiffuseInfoVec;

	// r: 给定半球上的 Theta 步幅
	// g: 给定半球上的 Phi   步幅
	// b: wi 的总采样数
	// a: roughness
	XMFLOAT4 IBLSpecularInfoVec;

	// r: NdotV 的步幅
	// g: roughness 的步幅
	// b: 采样数
	XMFLOAT4 IBLBSDFInfoVec;

};

struct CBGrassInfo
{
	XMFLOAT4	cameraPositionW;
	XMFLOAT4	grassSize;
	XMFLOAT4	windTime;
	XMFLOAT4	maxDepth;
	XMFLOAT4X4	vp;
};



