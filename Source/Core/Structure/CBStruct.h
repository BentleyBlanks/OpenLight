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
	// r: wi �����ϵ� Theta ������ [0,PI/2]
	// g: wi �����ϵ� Phi   ������ [0,2PI]
	// b: ���������ϵ� Theta ����   
	// a: ���������ϵ� Phi   ����
	XMFLOAT4 IBLDiffuseInfoVec;

	// r: ���������ϵ� Theta ����
	// g: ���������ϵ� Phi   ����
	// b: wi ���ܲ�����
	// a: roughness
	XMFLOAT4 IBLSpecularInfoVec;

	// r: NdotV �Ĳ���
	// g: roughness �Ĳ���
	// b: ������
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



