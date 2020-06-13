#pragma once
#include "Resource/GPUResource.h"
#include "Structure/CBStruct.h"
#include<string>
#define IrradianceDiffuseThetaResolution 64
#define IrradianceDiffusePhiResolution 128
#define IrradianceDiffuseSampleThetaCount 32
#define IrradianceDiffuseSamplePhiCount 64
#define IrradianceSpecularPrefilterMipmapCount 5
#define IrradianceSpecularPrefilterThetaRes   256
#define IrradianceSpecularPrefilterPhiRes	   256
#define IrradianceSpecularBSDFNdotVRes		512
#define IrradianceSpecularBSDFRoughnessRes  512
#define IrradianceSpecularBSDFSampleCount   2048
struct EnvironmentMap
{
	EnvironmentMap() {}
	~EnvironmentMap()
	{
		ReleaseCom(image);
		ReleaseCom(diffuseIrradianceMap);
		ReleaseCom(specularPrefilterMap);
		ReleaseCom(specularBSDFMap);
		ReleaseCom(cbIBLInfo);
		ReleaseCom(iblSignature);
	}
	std::string					name                          = {};
	ID3D12Resource*				image                         = nullptr;
	OpenLight::DescriptorIndex	srvIndex;

	ID3D12RootSignature*		iblSignature				  = nullptr;

	ID3D12Resource*				cbIBLInfo                     = nullptr;
	CBIBLInfo*					cbIBLInfoPtr				  = nullptr;
	OpenLight::DescriptorIndex	cbIBLInfoIndex                = {};


	OpenLight::DescriptorIndex	IBLSRVIndex                   = {};
	// Diffuse Irradiacne Map 
	ID3D12Resource*				diffuseIrradianceMap          = nullptr;
	OpenLight::DescriptorIndex  diffuseIrradianceMapUAVIndex  = {};

	// Specular 
	// Ô¤ÂË²¨»·¾³ÌùÍ¼
	// \int_{Li(wi,n)dwi}
	ID3D12Resource*				specularPrefilterMap          = nullptr;
	std::array<OpenLight::DescriptorIndex, IrradianceSpecularPrefilterMipmapCount> specularPrefilterMapUAVs;

	// BSDF ¾í»ý
	ID3D12Resource*				specularBSDFMap               = nullptr;
	OpenLight::DescriptorIndex  specularBSDFMapUAVIndex       = {};



};

struct SkyBox
{
	SkyBox() {}
	~SkyBox()
	{
		ReleaseCom(vb);
		ReleaseCom(ib);
		ReleaseCom(skyBoxSignature);
		ReleaseCom(skyBoxPSORGB32);
	}


	EnvironmentMap						envMap;

	ID3D12Resource*						vb = nullptr;
	ID3D12Resource*						ib = nullptr;
	D3D12_VERTEX_BUFFER_VIEW			skyBoxVBView;
	D3D12_INDEX_BUFFER_VIEW				skyBoxIBView;
	std::shared_ptr<ObjMesh>			skyBoxMesh = nullptr;
	ID3D12RootSignature*				skyBoxSignature = nullptr;
	ID3D12PipelineState*				skyBoxPSORGB32 = nullptr;
};