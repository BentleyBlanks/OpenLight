#include "Shader.h"
#include <string>
#include <fstream>
#include <d3dcompiler.h>
#include <iostream>
#include <DirectXMath.h>
#include "Utility.h"
#include <d3dx12.h>

void LoadFile(const char* path, std::string& str)
{
	if (!path)	return;

	std::ifstream f(path);
	f.seekg(0 , std::ios::end);
	str.reserve(f.tellg());
	f.seekg(0 , std::ios::beg);

	str.assign(std::istreambuf_iterator<char>(f) , std::istreambuf_iterator<char>());
}

Shader::Shader(ID3D12Device2* Device, const char* path, const char* vsEntry, const char* psEntry)
{
	std::string shaderContent;
	LoadFile(path , shaderContent);

	ID3DBlob* errorBlob = nullptr;
	ID3DBlob* vertexShaderBlob = nullptr;
	HRESULT result = D3DCompile(shaderContent.c_str() ,
								shaderContent.size()  ,
								path                  ,
								nullptr               ,
								nullptr               ,
								vsEntry               ,
								"vs_5_1"              ,
								0                     ,
								0                     ,
								&vertexShaderBlob     ,
								&errorBlob);

	if (FAILED(result))
	{
		if (errorBlob) std::cout << "vertex shader compile failed: " << (char*)errorBlob->GetBufferPointer() << std::endl;
		return;
	}

	ID3DBlob* pixelShaderBlob = nullptr;
	result = D3DCompile(shaderContent.c_str() ,
						shaderContent.size()  ,
						path                  ,
						nullptr               ,
						nullptr               ,
						psEntry               ,
						"ps_5_1"              ,
						0                     ,
						0                     ,
						&pixelShaderBlob      ,
						&errorBlob);
	if (FAILED(result))
	{
		if(errorBlob)	std::cout << "pixel shader compile failed: " << (char*)errorBlob->GetBufferPointer() << std::endl;
		return;
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR"	, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureData = {};
	FeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE , &FeatureData , sizeof(FeatureData))))
	{
		FeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	D3D12_ROOT_PARAMETER1 RootParameters[1];
	RootParameters[0].ParameterType    = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	RootParameters->Constants.Num32BitValues = sizeof(DirectX::XMMATRIX) / 4;
	RootParameters->Constants.ShaderRegister = 0;
	RootParameters->Constants.RegisterSpace  = 0;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
													D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
													D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
													D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS   |
													D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSignatureDescription.Desc_1_1.NumParameters = 1;
	rootSignatureDescription.Desc_1_1.pParameters = RootParameters;
	rootSignatureDescription.Desc_1_1.NumStaticSamplers = 0;
	rootSignatureDescription.Desc_1_1.pStaticSamplers = nullptr;
	rootSignatureDescription.Desc_1_1.Flags = rootSignatureFlags;

	ID3DBlob* rootSignatureBlob = nullptr;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription  , 
													    FeatureData.HighestVersion ,
													    &rootSignatureBlob         ,
													    &errorBlob));
	
	ThrowIfFailed(Device->CreateRootSignature(0 ,
											  rootSignatureBlob->GetBufferPointer() ,
											  rootSignatureBlob->GetBufferSize() ,
											  IID_PPV_ARGS(&mRootSignature)));

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT	 InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	}pipelineStateStream;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature        = mRootSignature;
	pipelineStateStream.InputLayout           = {inputLayout, _countof(inputLayout)};
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS                    = CD3DX12_SHADER_BYTECODE(vertexShaderBlob);
	pipelineStateStream.PS                    = CD3DX12_SHADER_BYTECODE(pixelShaderBlob);
	pipelineStateStream.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats            = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {sizeof(pipelineStateStream), &pipelineStateStream};
	ThrowIfFailed(Device->CreatePipelineState(&pipelineStateStreamDesc , IID_PPV_ARGS(&mPipelineState)));
}

ID3D12RootSignature* Shader::GetRootSignature() const
{
	return mRootSignature;
}

ID3D12PipelineState* Shader::GetPipelineState() const
{
	return mPipelineState;
}
