#include "DeferredShadingRenderGraphDemo.h"
#include "Utility.h"
#include "TextureManager.h"
#include <chrono>
#include "Structure/Terrain.h"
extern void UpdateRenderTargetViews(ID3D12Device5* device, IDXGISwapChain4* swapChain, ID3D12DescriptorHeap* descriptorHeap, ID3D12Resource* backBufferList[AppConfig::NumFrames]);




struct QuadVertex
{
	XMFLOAT3 positionL;
	XMFLOAT2 texcoord;
};
DeferredShadingRenderGraphDemo::DeferredShadingRenderGraphDemo()
{
	mCamera = new RoamCamera(
		XMFLOAT3(0, 0, 1),
		XMFLOAT3(0, 1, 0),
		XMFLOAT3(0, 5, -40),
		0.01f,
		1000.f,
		AppConfig::ClientWidth,
		AppConfig::ClientHeight);
}

DeferredShadingRenderGraphDemo::~DeferredShadingRenderGraphDemo()
{
	ReleaseCom(mConstructGBufferRootSignature);
	ReleaseCom(mConstructGBufferPSO);
	ReleaseCom(mDeferredLightingRootSignature);
	ReleaseCom(mDeferredLightingPSO);
	ReleaseCom(mConstructGBufferTerrainRootSignature);
	ReleaseCom(mConstructGBufferTerrainPSO);
	ReleaseCom(mDepthPassRootSignature);
	ReleaseCom(mDepthPassPSO);

	ReleaseCom(mPostprocessSignature);
	ReleaseCom(mPostprocessPSO);
	ReleaseCom(mQuadVB);
	ReleaseCom(mQuadIB);

	delete mUploadHeap;
	delete mDescriptorHeap;
	delete mCamera;
}

void DeferredShadingRenderGraphDemo::Init(HWND hWnd)
{

	mTimer.reset();
	Renderer::Init(hWnd);

	mDescriptorHeap = new GPUDescriptorHeapWrap(mDevice);
	// ���� InputLayout
	D3D12_INPUT_ELEMENT_DESC inputDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// ���ļ��м���ģ��
	std::shared_ptr<MeshPkg> bunny(new MeshPkg);
	bunny->mesh = ObjMeshLoader::loadObjMeshFromFile("F:\\OpenLight\\Resource\\cube.obj");
	UINT modelSize =
		bunny->mesh->submeshs[0].vertices.size() * sizeof(StandardVertex) +
		bunny->mesh->submeshs[0].indices.size() * sizeof(UINT);
	// ���� Upload Heap
	{
		mUploadHeap = new OpenLight::GPUUploadHeapWrap(mDevice);
	}

	// ���� VB IB
	{

		UINT verticesSizeInBytes = bunny->mesh->submeshs[0].vertices.size() * sizeof(StandardVertex);
		UINT indicesSizeInBytes = bunny->mesh->submeshs[0].indices.size() * sizeof(UINT);

		bunny->vb = mUploadHeap->createResource(mDevice, verticesSizeInBytes,
			CD3DX12_RESOURCE_DESC::Buffer(verticesSizeInBytes));


		bunny->vbView.BufferLocation = bunny->vb->GetGPUVirtualAddress();

		bunny->vbView.StrideInBytes = sizeof(StandardVertex);
		bunny->vbView.SizeInBytes = verticesSizeInBytes;

		void* p = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(bunny->vb->Map(0, &readRange, &p));
		std::memcpy(p, &bunny->mesh->submeshs[0].vertices[0], verticesSizeInBytes);
		bunny->vb->Unmap(0, nullptr);

		bunny->ib = mUploadHeap->createResource(mDevice,
			indicesSizeInBytes,
			CD3DX12_RESOURCE_DESC::Buffer(indicesSizeInBytes));

		bunny->ibView.BufferLocation = bunny->ib->GetGPUVirtualAddress();
		bunny->ibView.SizeInBytes = indicesSizeInBytes;
		bunny->ibView.Format = DXGI_FORMAT_R32_UINT;

		ThrowIfFailed(bunny->ib->Map(0, &readRange, &p));
		std::memcpy(p, &bunny->mesh->submeshs[0].indices[0], indicesSizeInBytes);
		bunny->ib->Unmap(0, nullptr);

	}
	mPBRMeshs.push_back(bunny);

	// ���� diffuse ����
	{
		ThrowIfFailed(mCommandAllocator[0]->Reset());
		ThrowIfFailed(mCommandList->Reset(mCommandAllocator[0], nullptr));
		m39DiffuseMap = TextureMgr::LoadTexture2DFromFile2("F:\\OpenLight\\Resource\\39txd1.jpg",
			mDevice,
			mCommandList,
			mCommandQueue);
	
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		m39DescriptorIndex = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mDescriptorHeap->AddSRV(&m39DescriptorIndex, m39DiffuseMap, srvDesc);
	}
	
	// ���� CPUFrameResource
	for (int i = 0; i < AppConfig::NumFrames; ++i)
	{
		mCPUFrameResource.push_back(std::make_unique<DeferredShadingRenderGraphFrameResource>(mDevice, mCommandAllocator[i], mDescriptorHeap));
	}




	// ���� Shader
	WRL::ComPtr<ID3DBlob> depthVS;
	WRL::ComPtr<ID3DBlob> depthPS;
	WRL::ComPtr<ID3DBlob> gbufferVS;
	WRL::ComPtr<ID3DBlob> gbufferPS;
	WRL::ComPtr<ID3DBlob> terrainGBufferVS;
	WRL::ComPtr<ID3DBlob> terrainGBufferPS;
	WRL::ComPtr<ID3DBlob> deferredLightingVS;
	WRL::ComPtr<ID3DBlob> deferredLightingPS;

#if defined(_DEBUG)		
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif 

	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\DepthPassVS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "DepthMainVS", "vs_5_0", compileFlags, 0, &depthVS, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\DepthPassPS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "DepthMainPS", "ps_5_0", compileFlags, 0, &depthPS, nullptr));

	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\DeferredGBufferVS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &gbufferVS, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\DeferredGBufferPS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &gbufferPS, nullptr));

	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\TerrainVS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "TerrainMainVS", "vs_5_0", compileFlags, 0, &terrainGBufferVS, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\TerrainPS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "TerrainGBufferMainPS", "ps_5_0", compileFlags, 0, &terrainGBufferPS, nullptr));

	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\DeferredLightingVS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "DeferredLightingVSMain", "vs_5_0", compileFlags, 0, &deferredLightingVS, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\DeferredLightingPS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "DeferredLightingPSMain", "ps_5_0", compileFlags, 0, &deferredLightingPS, nullptr));

	// Depth Pass RootSignature PSO
	{
		CD3DX12_ROOT_PARAMETER1 rootParams[1];
		rootParams[0].InitAsConstantBufferView(0);

		WRL::ComPtr<ID3DBlob> pSignatureBlob;
		WRL::ComPtr<ID3DBlob> pErrorBlob;
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParams), rootParams,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		// ������ǩ��
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_1,
			&pSignatureBlob,
			&pErrorBlob));
		ThrowIfFailed(mDevice->CreateRootSignature(0,
			pSignatureBlob->GetBufferPointer(),
			pSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&mDepthPassRootSignature)));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputDesc, _countof(inputDesc) };
		psoDesc.pRootSignature = mDepthPassRootSignature;
		psoDesc.VS.pShaderBytecode = depthVS->GetBufferPointer();
		psoDesc.VS.BytecodeLength = depthVS->GetBufferSize();
		psoDesc.PS.pShaderBytecode = depthPS->GetBufferPointer();
		psoDesc.PS.BytecodeLength = depthPS->GetBufferSize();
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 0;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mDepthPassPSO)));

	}

	// GBuffer Pass RootSignature PSO
	{
		CD3DX12_ROOT_PARAMETER1 rootParameters[3];
		CD3DX12_DESCRIPTOR_RANGE1 descRange[2];
		descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &descRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[2].InitAsDescriptorTable(1, &descRange[1], D3D12_SHADER_VISIBILITY_PIXEL);

		WRL::ComPtr<ID3DBlob> pSignatureBlob;
		WRL::ComPtr<ID3DBlob> pErrorBlob;
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		// ������ǩ��
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_1,
			&pSignatureBlob,
			&pErrorBlob));
		ThrowIfFailed(mDevice->CreateRootSignature(0,
			pSignatureBlob->GetBufferPointer(),
			pSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&mConstructGBufferRootSignature)));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputDesc, _countof(inputDesc) };
		psoDesc.pRootSignature = mConstructGBufferRootSignature;
		psoDesc.VS.pShaderBytecode = gbufferVS->GetBufferPointer();
		psoDesc.VS.BytecodeLength = gbufferVS->GetBufferSize();
		psoDesc.PS.pShaderBytecode = gbufferPS->GetBufferPointer();
		psoDesc.PS.BytecodeLength = gbufferPS->GetBufferSize();
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mConstructGBufferPSO)));

	}

	// Terrain Pass RootSignature PSO
	{
		CD3DX12_ROOT_PARAMETER1 rootParameters[3];
		CD3DX12_DESCRIPTOR_RANGE1 descRange[2];
		descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &descRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[2].InitAsDescriptorTable(1, &descRange[1], D3D12_SHADER_VISIBILITY_PIXEL);

		WRL::ComPtr<ID3DBlob> pSignatureBlob;
		WRL::ComPtr<ID3DBlob> pErrorBlob;
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		// ������ǩ��
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_1,
			&pSignatureBlob,
			&pErrorBlob));
		ThrowIfFailed(mDevice->CreateRootSignature(0,
			pSignatureBlob->GetBufferPointer(),
			pSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&mConstructGBufferTerrainRootSignature)));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputDesc, _countof(inputDesc) };
		psoDesc.pRootSignature = mConstructGBufferTerrainRootSignature;
		psoDesc.VS.pShaderBytecode = terrainGBufferVS->GetBufferPointer();
		psoDesc.VS.BytecodeLength = terrainGBufferVS->GetBufferSize();
		psoDesc.PS.pShaderBytecode = terrainGBufferPS->GetBufferPointer();
		psoDesc.PS.BytecodeLength = terrainGBufferPS->GetBufferSize();
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mConstructGBufferTerrainPSO)));
	}

	// Deferred Pass RootSignature PSO
	{
		D3D12_INPUT_ELEMENT_DESC inputDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		CD3DX12_DESCRIPTOR_RANGE1 descRange[2];
		descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
		descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
		rootParameters[0].InitAsDescriptorTable(1, &descRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &descRange[1], D3D12_SHADER_VISIBILITY_PIXEL);

		WRL::ComPtr<ID3DBlob> pSignatureBlob;
		WRL::ComPtr<ID3DBlob> pErrorBlob;
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		// ������ǩ��
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_1,
			&pSignatureBlob,
			&pErrorBlob));
		ThrowIfFailed(mDevice->CreateRootSignature(0,
			pSignatureBlob->GetBufferPointer(),
			pSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&mDeferredLightingRootSignature)));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputDesc, _countof(inputDesc) };
		psoDesc.pRootSignature = mDeferredLightingRootSignature;
		psoDesc.VS.pShaderBytecode = deferredLightingVS->GetBufferPointer();
		psoDesc.VS.BytecodeLength = deferredLightingVS->GetBufferSize();
		psoDesc.PS.pShaderBytecode = deferredLightingPS->GetBufferPointer();
		psoDesc.PS.BytecodeLength = deferredLightingPS->GetBufferSize();
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mDeferredLightingPSO)));
	}
	

	mViewport = { 0.0f, 0.0f
		, static_cast<float>(AppConfig::ClientWidth), static_cast<float>(AppConfig::ClientHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	mScissorRect = { 0, 0
		, static_cast<LONG>(AppConfig::ClientWidth), static_cast<LONG>(AppConfig::ClientHeight) };


	// ���� Sampler
	D3D12_SAMPLER_DESC samplerDesc = {  };
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MinLOD = -D3D12_FLOAT32_MAX;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	mSamplerIndices = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	mDescriptorHeap->AddSampler(&mSamplerIndices, samplerDesc);
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	mSamPointIndex = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	mDescriptorHeap->AddSampler(&mSamPointIndex, samplerDesc);



	InitPostprocess();
	//	InitIBL();
	ThrowIfFailed(mCommandAllocator[0]->Reset());
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator[0], nullptr));
	mTerrain = std::shared_ptr<Terrain>(new Terrain(mDevice, mCommandList, mCommandQueue, mDescriptorHeap, "F:\\OpenLight\\Resource\\heightmap256.png", 20.f, 1, 1));
	// ��ʼ�����ʺ͹�Դ
	{
		mPointLights.lightPositions[0] = XMFLOAT4(50, 10, -20, 1.f);
		mPointLights.lightIntensitys[0] = XMFLOAT4(5000, 5000, 5000, 1.f);
		mPointLights.lightPositions[1] = XMFLOAT4(20, -20, 20, 1.f);
		mPointLights.lightIntensitys[1] = XMFLOAT4(0, 0, 0, 1.f);
		mPointLights.lightPositions[2] = XMFLOAT4(-20, 20, 20, 1.f);
		mPointLights.lightIntensitys[2] = XMFLOAT4(0, 100, 100, 1.f);
		mPointLights.lightPositions[3] = XMFLOAT4(20, 20, 20, 1.f);
		mPointLights.lightIntensitys[3] = XMFLOAT4(0, 0, 0, 1.f);
		mPointLights.lightSizeFloat = XMFLOAT4(4.f, 4.f, 4.f, 4.f);

		bunny->world = XMFLOAT4X4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		);
		bunny->material.materialAlbedo = XMFLOAT4(1, 0, 0, 0.5f);
		bunny->material.materialInfo = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f);

		mTerrain->world = XMFLOAT4X4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		);
	}

	InitRenderGraph();

}

void DeferredShadingRenderGraphDemo::Render()
{
	Update();
	GraphResourceMgr::GetInstance()->BeginFrame();
	auto commandAllocator = mCommandAllocator[mCurrentBackBufferIndex];
	auto backBuffer = mBackBuffer[mCurrentBackBufferIndex];


	commandAllocator->Reset();
	mCommandList->Reset(commandAllocator, nullptr);
	static bool firstCreateNoise = true;

	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// �л� ��RTV ��״̬ 
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1, &barrier);

	//DepthPass();
	//GBuffer();
	//Lighting();
	GraphExecutor::GetInstance()->Execute(mGraphCompiledResult);

	// GUI
	renderGUI();

	// Present
	barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1, &barrier);

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* const commandLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	UINT syncInterval = AppConfig::VSync ? 1 : 0;
	UINT presentFlags = AppConfig::TearingSupported && !AppConfig::VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(mSwapChain->Present(syncInterval, presentFlags));

	mCPUFrameResource[mCurrentBackBufferIndex]->Fence = Signal(mCommandQueue, mFence, mFenceValue);


	// ���� CurrentBackBufferIndex
	mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % AppConfig::NumFrames;


}

void DeferredShadingRenderGraphDemo::Resize(uint32_t width, uint32_t height)
{
	if (AppConfig::ClientWidth != width || AppConfig::ClientHeight != height)
	{
		AppConfig::ClientWidth = std::max(1u, width);
		AppConfig::ClientHeight = std::max(1u, height);

		Flush(mCommandQueue, mFence, mFenceValue, mFenceEvent);

		for (int i = 0; i < AppConfig::NumFrames; i++)
		{
			mBackBuffer[i]->Release();
			mCPUFrameResource[i]->Fence = mCPUFrameResource[mCurrentBackBufferIndex]->Fence;
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(mSwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(mSwapChain->ResizeBuffers(AppConfig::NumFrames, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

		UpdateRenderTargetViews(mDevice, mSwapChain, mRTVDescriptorHeap, mBackBuffer);
	}
}

void DeferredShadingRenderGraphDemo::Update()
{
	FPS();
	mTimer.tick();
	static float runTime = 0.f;


	// �������
	static float speed = 20.f;
	float dt = mTimer.deltaTime();
	runTime += dt;
	mDeltaTime += dt;
	if (GetAsyncKeyState('W') & 0x8000)
		mCamera->walk(speed * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera->walk(-speed * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera->strafe(-speed * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera->strafe(speed * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera->yaw(+5.f * dt);
	if (GetAsyncKeyState('E') & 0x8000)
		mCamera->yaw(-5.f * dt);

	auto& IO = ImGui::GetIO();
	if (IO.MouseDown[2])
	{

		{

			float dx = XMConvertToRadians(0.25f * IO.MouseDelta.x);
			float dy = XMConvertToRadians(0.25f * IO.MouseDelta.y);
			mCamera->pitch(dy);
			mCamera->rotateY(dx);
		}

	}

	mCamera->updateMatrix();


	if (mCPUFrameResource[mCurrentBackBufferIndex]->Fence != 0 && mFence->GetCompletedValue() < mCPUFrameResource[mCurrentBackBufferIndex]->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCPUFrameResource[mCurrentBackBufferIndex]->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}


	XMMATRIX vp = mCamera->getViewProj();
	XMMATRIX world = XMLoadFloat4x4(&mPBRMeshs[0]->world);
	XMFLOAT3 cameraPos = mCamera->getPos();

	XMStoreFloat4x4(&mCPUFrameResource[mCurrentBackBufferIndex]->cbTransPtr->wvp, XMMatrixTranspose(world * vp));
	XMStoreFloat4x4(&mCPUFrameResource[mCurrentBackBufferIndex]->cbTransPtr->world, XMMatrixTranspose(world));
	XMStoreFloat4x4(&mCPUFrameResource[mCurrentBackBufferIndex]->cbTransPtr->invTranspose, XMMatrixTranspose(world));

	world = XMMatrixScaling(10, 10, 10);

	mCPUFrameResource[mCurrentBackBufferIndex]->cbCameraPtr->cameraPositionW = XMFLOAT4(cameraPos.x, cameraPos.y, cameraPos.z, 1.f);

	world = XMLoadFloat4x4(&mTerrain->world);
	XMStoreFloat4x4(&mCPUFrameResource[mCurrentBackBufferIndex]->cbTerrainTransPtr->wvp, XMMatrixTranspose(world * vp));
	XMStoreFloat4x4(&mCPUFrameResource[mCurrentBackBufferIndex]->cbTerrainTransPtr->world, XMMatrixTranspose(world));
	XMStoreFloat4x4(&mCPUFrameResource[mCurrentBackBufferIndex]->cbTerrainTransPtr->invTranspose, XMMatrixTranspose(world));





	// GUI
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		{
			if (!ImGui::Begin("UI"))
			{
				ImGui::End();
			}
			else
			{
				ImGui::BeginChild("World");

				if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
				{


					// ���ʴ���
#if 0
					if (ImGui::BeginTabItem("Material"))
					{
						// ��ʾ������Ϣ
//						auto& material = mPBRMeshs[0]->material;
						auto& material = mTerrain->material;
						if (ImGui::DragFloat4("Albedo", (float*)&material.materialAlbedo, 0.01f, 0.f, 1.f))
							mPBRMeshs[0]->materialChanged = true;

						ImGui::SameLine();
						if (ImGui::Combo("Default", &mSelectedFresnelIndex, AppConfig::GlobalFresnelMaterialName, EFresnelMaterialNameCount))
						{
							mTerrain->material.materialAlbedo = AppConfig::GlobalFresnelMaterial[mSelectedFresnelIndex];
						}

						if (ImGui::DragFloat("Roughness", &material.materialInfo.x, 0.01f, 0.f, 1.f))
							mPBRMeshs[0]->materialChanged = true;
						if (ImGui::DragFloat("Metallic", &material.materialInfo.y, 0.01f, 0.f, 1.f))
							mPBRMeshs[0]->materialChanged = true;
						if (ImGui::DragFloat("AO", &material.materialInfo.z, 0.01f, 0.f, 1.f))
							mPBRMeshs[0]->materialChanged = true;



						ImGui::EndTabItem();

					}
#endif // 0

					static const char* pointLightName[] = { "Light0","Light1","Light2","Light3" };
					// ���Դ����
					if (ImGui::BeginTabItem("PointLights"))
					{
						ImGui::Combo("Light", &mSelectedLightIndex, pointLightName, 4);
						ImGui::Separator();

						// ��ʾ��Դ��Ϣ

						auto& lightPosition = mPointLights.lightPositions[mSelectedLightIndex];
						auto& lightIntensity = mPointLights.lightIntensitys[mSelectedLightIndex];

						ImGui::DragFloat3("Position", (float*)(&lightPosition));
						ImGui::DragFloat3("Intensity", (float*)(&lightIntensity), 1);

						ImGui::EndTabItem();
					}
	


					ImGui::EndTabBar();

				}

				ImGui::EndChild();


				ImGui::End();
			}
		}
	}

}

void DeferredShadingRenderGraphDemo::InitPostprocess()
{
	// ���� InputLayout
	D3D12_INPUT_ELEMENT_DESC inputDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

#if defined(_DEBUG)		
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif 
	WRL::ComPtr<ID3DBlob> vs;
	WRL::ComPtr<ID3DBlob> ps;

	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\PostprocessVS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "PostprocessVSMain", "vs_5_0", compileFlags, 0, &vs, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\PostprocessPS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "GammaCorrectPSMain", "ps_5_0", compileFlags, 0, &ps, nullptr));

	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	CD3DX12_DESCRIPTOR_RANGE1 descRange[1];
	descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
	rootParameters[0].InitAsDescriptorTable(1, &descRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MinLOD = 0.f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init_1_1(1, rootParameters,
		1, &samplerDesc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	// ������ǩ��
	WRL::ComPtr<ID3DBlob> pSignatureBlob;
	WRL::ComPtr<ID3DBlob> pErrorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_1,
		&pSignatureBlob,
		&pErrorBlob));
	ThrowIfFailed(mDevice->CreateRootSignature(0,
		pSignatureBlob->GetBufferPointer(),
		pSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&mPostprocessSignature)));

	// ���� PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputDesc, _countof(inputDesc) };
	psoDesc.pRootSignature = mPostprocessSignature;
	psoDesc.VS.pShaderBytecode = vs->GetBufferPointer();
	psoDesc.VS.BytecodeLength = vs->GetBufferSize();
	psoDesc.PS.pShaderBytecode = ps->GetBufferPointer();
	psoDesc.PS.BytecodeLength = ps->GetBufferSize();
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPostprocessPSO)));


	std::vector<QuadVertex> vertices(4);
	std::vector<UINT>		indices;

	vertices[0].positionL = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	vertices[1].positionL = XMFLOAT3(-1.0f, +1.0f, 0.0f);
	vertices[2].positionL = XMFLOAT3(+1.0f, +1.0f, 0.0f);
	vertices[3].positionL = XMFLOAT3(+1.0f, -1.0f, 0.0f);
	// Store far plane frustum corner indices in normal.x slot.

	vertices[0].texcoord = XMFLOAT2(0.0f, 1.0f);
	vertices[1].texcoord = XMFLOAT2(0.0f, 0.0f);
	vertices[2].texcoord = XMFLOAT2(1.0f, 0.0f);
	vertices[3].texcoord = XMFLOAT2(1.0f, 1.0f);
	indices = { 0,1,2,0,2,3 };

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(QuadVertex) * vertices.size()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mQuadVB)));

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * indices.size()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mQuadIB)));

	{
		void* p;
		ThrowIfFailed(mQuadVB->Map(0, nullptr, &p));
		std::memcpy(p, &vertices[0], sizeof(QuadVertex) * vertices.size());
		mQuadVB->Unmap(0, nullptr);

		ThrowIfFailed(mQuadIB->Map(0, nullptr, &p));
		std::memcpy(p, &indices[0], sizeof(UINT) * indices.size());
		mQuadIB->Unmap(0, nullptr);
	}
	mQuadVBView.BufferLocation = mQuadVB->GetGPUVirtualAddress();
	mQuadVBView.StrideInBytes = sizeof(QuadVertex);
	mQuadVBView.SizeInBytes = sizeof(QuadVertex) * vertices.size();

	mQuadIBView.BufferLocation = mQuadIB->GetGPUVirtualAddress();
	mQuadIBView.Format = DXGI_FORMAT_R32_UINT;
	mQuadIBView.SizeInBytes = sizeof(UINT) * indices.size();

}

void DeferredShadingRenderGraphDemo::InitSkyBox()
{
}


void DeferredShadingRenderGraphDemo::FPS()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((mTimer.totalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText =
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowTextW(mHWND, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void DeferredShadingRenderGraphDemo::RenderSkyBox()
{

}


void DeferredShadingRenderGraphDemo::DepthPass(const RenderGraphPass* pass)
{
	// Set RootSignature and PSO
	mCommandList->SetGraphicsRootSignature(mDepthPassRootSignature);
	mCommandList->SetPipelineState(mDepthPassPSO);

	// Set Descriptor Heap
	ID3D12DescriptorHeap* ppHeaps[] = { mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// ���� DSV
	mCommandList->OMSetRenderTargets(0, nullptr, FALSE, &mDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// ���� CBV
	auto& currentFrameResource = mCPUFrameResource[mCurrentBackBufferIndex];
	mCommandList->SetGraphicsRootConstantBufferView(0, currentFrameResource->cbTrans->GetGPUVirtualAddress());

	// IA Setups
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mPBRMeshs[0]->vbView);
	mCommandList->IASetIndexBuffer(&mPBRMeshs[0]->ibView);

	// Draw Call !!
	mCommandList->DrawIndexedInstanced(mPBRMeshs[0]->mesh->submeshs[0].indices.size(), 1, 0, 0, 0);

	mCommandList->IASetVertexBuffers(0, 1, &mTerrain->vbView);
	mCommandList->IASetIndexBuffer(&mTerrain->ibView);
	mCommandList->DrawIndexedInstanced(mTerrain->mIndices.size(), 1, 0, 0, 0);
	
}

void DeferredShadingRenderGraphDemo::GBuffer(const RenderGraphPass* pass)
{
	auto dynamicHeap = GPUDynamicDescriptorHeapWrap::GetInstance();
	auto resourceMgr = GraphResourceMgr::GetInstance();
	auto gbuffer0RTV = pass->GetDescriptorResource(mGBuffer0ID);
	auto gbuffer1RTV = pass->GetDescriptorResource(mGBuffer1ID);
	

	
	// Set RootSignature and PSO
	mCommandList->SetGraphicsRootSignature(mConstructGBufferRootSignature);
	mCommandList->SetPipelineState(mConstructGBufferPSO);


	// Set Descriptor Heap
	ID3D12DescriptorHeap* ppHeaps[] = { mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV), mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// ���� RTV DSV
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = { gbuffer0RTV->cpuHandle,gbuffer1RTV->cpuHandle };
	mCommandList->OMSetRenderTargets(2, rtvs, FALSE, &mDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	FLOAT clearColor[] = { 0.f,0.f,0.f,1.f };
	mCommandList->ClearRenderTargetView(rtvs[0], clearColor, 0, nullptr);	
	mCommandList->ClearRenderTargetView(rtvs[1], clearColor, 0, nullptr);


	auto& currentFrameResource = mCPUFrameResource[mCurrentBackBufferIndex];

	// Set CBV SRV Sampler
	mCommandList->SetGraphicsRootConstantBufferView(0, currentFrameResource->cbTrans->GetGPUVirtualAddress());
	mCommandList->SetGraphicsRootDescriptorTable(1, mDescriptorHeap->GPUHandle(m39DescriptorIndex));
	mCommandList->SetGraphicsRootDescriptorTable(2, mDescriptorHeap->GPUHandle(mSamplerIndices));

	// IA Setups
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mPBRMeshs[0]->vbView);
	mCommandList->IASetIndexBuffer(&mPBRMeshs[0]->ibView);

	// Draw Call !!
	mCommandList->DrawIndexedInstanced(mPBRMeshs[0]->mesh->submeshs[0].indices.size(), 1, 0, 0, 0);


	// Terrain
	GBufferTerrain(pass);

}

void DeferredShadingRenderGraphDemo::GBufferTerrain(const RenderGraphPass* pass)
{
	mCommandList->SetGraphicsRootSignature(mConstructGBufferTerrainRootSignature);
	mCommandList->SetPipelineState(mConstructGBufferTerrainPSO);

	ID3D12DescriptorHeap* ppHeaps[] = { mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
	mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	auto& frameResource = mCPUFrameResource[mCurrentBackBufferIndex];

	mCommandList->SetGraphicsRootConstantBufferView(0, frameResource->cbTerrainTrans->GetGPUVirtualAddress());
	mCommandList->SetGraphicsRootDescriptorTable(1,
		mDescriptorHeap->GPUHandle(mTerrain->diffuseSRVIndex));
	mCommandList->SetGraphicsRootDescriptorTable(2,
		mDescriptorHeap->GPUHandle(mSamplerIndices));



	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mTerrain->vbView);
	mCommandList->IASetIndexBuffer(&mTerrain->ibView);
	//Draw Call������
	mCommandList->DrawIndexedInstanced(mTerrain->mIndices.size(), 1, 0, 0, 0);
}

void DeferredShadingRenderGraphDemo::Lighting(const RenderGraphPass* pass)
{

	auto dynamicHeap = GPUDynamicDescriptorHeapWrap::GetInstance();
	auto resourceMgr = GraphResourceMgr::GetInstance();
	auto gbufferSRV = pass->GetDescriptorResource(mGBuffer0ID);
	auto sceneRTV = pass->GetDescriptorResource(mSceneBufferID);

	// Set RootSignature and PSO
	mCommandList->SetGraphicsRootSignature(mDeferredLightingRootSignature);
	mCommandList->SetPipelineState(mDeferredLightingPSO);


	// Set Descriptor Heap
	ID3D12DescriptorHeap* ppHeaps[] = { dynamicHeap->GetGPUDescriptorHeap(), mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = { sceneRTV->cpuHandle };
	mCommandList->OMSetRenderTargets(1, rtvs, FALSE, &mDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	FLOAT clearColor[] = { 0.f,0.f,0.f,1.f };
	mCommandList->ClearRenderTargetView(rtvs[0], clearColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(mDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);


	// Set RTV Sampler
	mCommandList->SetGraphicsRootDescriptorTable(0, gbufferSRV->gpuHandle);
	mCommandList->SetGraphicsRootDescriptorTable(1, mDescriptorHeap->GPUHandle(mSamplerIndices));


	// IA Setups
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mQuadVBView);
	mCommandList->IASetIndexBuffer(&mQuadIBView);
	//Draw Call������
	mCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void DeferredShadingRenderGraphDemo::Postprocess(const RenderGraphPass* pass)
{
	auto dynamicHeap = GPUDynamicDescriptorHeapWrap::GetInstance();
	auto resourceMgr = GraphResourceMgr::GetInstance();
	auto sceneSRV = pass->GetDescriptorResource(mSceneBufferID);

	mCommandList->SetGraphicsRootSignature(mPostprocessSignature);
	mCommandList->SetPipelineState(mPostprocessPSO);
	ID3D12DescriptorHeap* ppHeaps[] = { dynamicHeap->GetGPUDescriptorHeap() };
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	mCommandList->SetGraphicsRootDescriptorTable(0, sceneSRV->gpuHandle);

	auto rtv = (mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.ptr += mCurrentBackBufferIndex * mRTVDescriptorSize;
	mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	mCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mQuadVBView);
	mCommandList->IASetIndexBuffer(&mQuadIBView);
	//Draw Call������
	mCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void DeferredShadingRenderGraphDemo::InitRenderGraph()
{
	auto graphResourceMgr = GraphResourceMgr::GetInstance();
	auto graph = RenderGraph::GetInstance();
	// Create Logical Resource
	// Depth Buffer
	LogicalResourceID depthBuffer = graphResourceMgr->CreateLogicalResource("DepthBuffer");
	// GBuffer Buffer
	LogicalResourceID GBuffer0;
	LogicalResourceID GBuffer1;
	{
		PhysicalDesc physicalDesc;
		FLOAT clearColor[] = { 0.f,0.f,0.f,0.f };
 		physicalDesc.initialiValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		std::memcpy(&physicalDesc.initialiValue.Color, clearColor, sizeof(FLOAT) * 4);
		physicalDesc.desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, AppConfig::ClientWidth, AppConfig::ClientHeight,
			1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		GBuffer0 = graphResourceMgr->CreateLogicalResource("GBuffer0", physicalDesc);
		GBuffer1 = graphResourceMgr->CreateLogicalResource("GBuffer1", physicalDesc);
	}
	// Scene Buffer
	LogicalResourceID sceneBuffer;
	{
		PhysicalDesc physicalDesc;
		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		D3D12_CLEAR_VALUE d3dClearValue;
		physicalDesc.initialiValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		std::memcpy(&physicalDesc.initialiValue.Color, clearColor, sizeof(FLOAT) * 4);
		physicalDesc.desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, AppConfig::ClientWidth, AppConfig::ClientHeight,
			1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		sceneBuffer = graphResourceMgr->CreateLogicalResource("SceneBuffer", physicalDesc);
	}

	// Create Render Pass
	mDepthPassID       = graph->CreateRenderPass("DepthPass");
	mGBufferPassID     = graph->CreateRenderPass("GBufferPass");
	mLightingPassID    = graph->CreateRenderPass("LightingPass");
	mPostprocessPassID = graph->CreateRenderPass("PostprocessPass");

	// Bind Resource
	auto depthPass = graph->GetPass(mDepthPassID);
	depthPass->BindOutput({ depthBuffer });

	auto gbufferPass = graph->GetPass(mGBufferPassID);
	gbufferPass->BindInput({ depthBuffer });
	gbufferPass->BindOutput({ GBuffer0,GBuffer1 });

	auto lightingPass = graph->GetPass(mLightingPassID);
	lightingPass->BindInput({ GBuffer0,GBuffer1 });
	lightingPass->BindOutput({ sceneBuffer });

	auto postprocessPass = graph->GetPass(mPostprocessPassID);
	postprocessPass->BindInput({ sceneBuffer });

	// Load Physical Resource : DepthBuffer
	{
		PhysicalResource depthBufferResource;
		D3D12_CLEAR_VALUE optClear;
		optClear.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
		optClear.DepthStencil.Depth   = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		depthBufferResource.resource  = mDepthBuffer;
		depthBufferResource.initState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		depthBufferResource.state     = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		depthBufferResource.desc      = { CD3DX12_RESOURCE_DESC(mDepthBuffer->GetDesc()),optClear };
		depthBufferResource.type      = EPhysical_Explicit;
		auto depthBufferResourceID = graphResourceMgr->LoadPhysicalResource(depthBufferResource);
		graphResourceMgr->BindUnknownResource(depthBuffer, depthBufferResourceID);
	}


	// Bind Descriptor Resource
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		DescriptorDesc gbuffer0RTVDesc =
			DescriptorDesc(
				{ GBuffer0 },
				{ rtvDesc },
				{ D3D12_RESOURCE_STATE_RENDER_TARGET}
		);
		DescriptorDesc gbuffer1RTVDesc =
			DescriptorDesc(
				{ GBuffer1 },
				{ rtvDesc },
				{ D3D12_RESOURCE_STATE_RENDER_TARGET }
		);
		DescriptorDesc gbufferSRVDesc =
			DescriptorDesc(
				{ GBuffer0,GBuffer1 },
				{ srvDesc,srvDesc },
				{ D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE }
		);

		DescriptorDesc sceneBufferRTVDesc =
			DescriptorDesc(
				{ sceneBuffer },
				{ rtvDesc },
				{ D3D12_RESOURCE_STATE_RENDER_TARGET }
		);

		DescriptorDesc sceneBufferSRVDesc =
			DescriptorDesc(
				{ sceneBuffer },
				{ srvDesc },
				{ D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE }
		);
		
		gbufferPass->BindLogicalDescriptor(gbuffer0RTVDesc.boundLogicalResources, gbuffer0RTVDesc);
		gbufferPass->BindLogicalDescriptor(gbuffer1RTVDesc.boundLogicalResources, gbuffer1RTVDesc);
		lightingPass->BindLogicalDescriptor(gbufferSRVDesc.boundLogicalResources, gbufferSRVDesc);
		lightingPass->BindLogicalDescriptor(sceneBufferRTVDesc.boundLogicalResources, sceneBufferRTVDesc);
		postprocessPass->BindLogicalDescriptor(sceneBufferSRVDesc.boundLogicalResources, sceneBufferSRVDesc);
	}

	mDepthBufferID = depthBuffer;
	mGBuffer0ID = GBuffer0;
	mGBuffer1ID = GBuffer1;
	mSceneBufferID = sceneBuffer;

	// Bind Execute function
	depthPass->BindExecuteFunc(std::bind(&DeferredShadingRenderGraphDemo::DepthPass, this, std::placeholders::_1));
	gbufferPass->BindExecuteFunc(std::bind(&DeferredShadingRenderGraphDemo::GBuffer, this, std::placeholders::_1));
	lightingPass->BindExecuteFunc(std::bind(&DeferredShadingRenderGraphDemo::Lighting, this, std::placeholders::_1));
	postprocessPass->BindExecuteFunc(std::bind(&DeferredShadingRenderGraphDemo::Postprocess, this, std::placeholders::_1));

	// Compile RenderGraph
	mGraphCompiledResult = GraphCompiler::GetInstance()->Compile(*graph);
	

}