#include"CubeRenderer.h"
#include "Utility.h"
#include "TextureManager.h"
#include <chrono>
using namespace DirectX;
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif


int  gMouseX = 0;
int  gMouseY = 0;
int  gLastMouseX = 0;
int  gLastMouseY = 0;
bool gMouseDown = false;
bool gMouseMove = false;

CubeRenderer::CubeRenderer()
{
	mCamera = new RoamCamera(
		XMFLOAT3(0, 0, 1),
		XMFLOAT3(0, 1, 0),
		XMFLOAT3(0, 5, -40),
		0.1f,
		2000.f,
		AppConfig::ClientWidth,
		AppConfig::ClientHeight);
}

CubeRenderer::~CubeRenderer()
{
	if (mCamera)
		delete mCamera;
}



void CubeRenderer::Init(HWND hWnd)
{
	mTimer.reset();
	Renderer::Init(hWnd);
	mDescriptorHeap = new GPUDescriptorHeapWrap(mDevice);
	// 创建 InputLayout
	D3D12_INPUT_ELEMENT_DESC inputDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// 从文件中加载模型
	mCubeMesh = ObjMeshLoader::loadObjMeshFromFile("F:\\OpenLight\\Cube.obj");
	UINT modelSize =
		mCubeMesh->submeshs[0].vertices.size() * sizeof(StandardVertex) +
		mCubeMesh->submeshs[0].indices.size() * sizeof(UINT);
	// 创建 Upload Heap
	{
		D3D12_HEAP_DESC uploadDesc = {};
		uploadDesc.SizeInBytes = PAD((1<<20), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		uploadDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		uploadDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
		uploadDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		uploadDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		uploadDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

		ThrowIfFailed(mDevice->CreateHeap(&uploadDesc, IID_PPV_ARGS(&mUploadHeap)));
	}

	// 创建 VB IB CB
	{
		UINT verticesSizeInBytes = mCubeMesh->submeshs[0].vertices.size() * sizeof(StandardVertex);
		UINT indicesSizeInBytes = mCubeMesh->submeshs[0].indices.size() * sizeof(UINT);
		UINT offset = 0;
		ThrowIfFailed(mDevice->CreatePlacedResource(
			mUploadHeap.Get(),
			offset,
			&CD3DX12_RESOURCE_DESC::Buffer(verticesSizeInBytes),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mCubeMeshVB)));

		mCubeMeshVBView.BufferLocation = mCubeMeshVB->GetGPUVirtualAddress();
		mCubeMeshVBView.StrideInBytes = sizeof(StandardVertex);
		mCubeMeshVBView.SizeInBytes = verticesSizeInBytes;

		void* p = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(mCubeMeshVB->Map(0, &readRange, &p));
		std::memcpy(p, &mCubeMesh->submeshs[0].vertices[0], verticesSizeInBytes);
		mCubeMeshVB->Unmap(0,nullptr);

		offset = PAD(offset + verticesSizeInBytes, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
		ThrowIfFailed(mDevice->CreatePlacedResource(
			mUploadHeap.Get(),
			offset,
			&CD3DX12_RESOURCE_DESC::Buffer(indicesSizeInBytes),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mCubeMeshIB)));

		mCubeMeshIBView.BufferLocation = mCubeMeshIB->GetGPUVirtualAddress();
		mCubeMeshIBView.SizeInBytes = indicesSizeInBytes;
		mCubeMeshIBView.Format = DXGI_FORMAT_R32_UINT;

		ThrowIfFailed(mCubeMeshIB->Map(0, &readRange, &p));
		std::memcpy(p, &mCubeMesh->submeshs[0].indices[0], indicesSizeInBytes);
		mCubeMeshIB->Unmap(0, nullptr);

		offset = PAD(offset + indicesSizeInBytes, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		ThrowIfFailed(mDevice->CreatePlacedResource(
			mUploadHeap.Get(),
			offset,
			&CD3DX12_RESOURCE_DESC::Buffer(PAD(sizeof(CBTrans),256)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mCBTrans)));

		ThrowIfFailed(mCBTrans->Map(0, nullptr, reinterpret_cast<void**>(&mCBTransGPUPtr)));
	}

	


	// 创建 Shader
	WRL::ComPtr<ID3DBlob> vs = nullptr;
	WRL::ComPtr<ID3DBlob> ps = nullptr;

#if defined(_DEBUG)		
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif 

	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\TextureCubeVS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &vs, nullptr));


	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\TextureCubePS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &ps, nullptr));


	// 创建根签名参数
	CD3DX12_ROOT_PARAMETER1 rootParameters[3];
	CD3DX12_DESCRIPTOR_RANGE1 descRange[3];
	descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
	descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0);
	descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	rootParameters[0].InitAsDescriptorTable(1, &descRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsDescriptorTable(1, &descRange[1], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &descRange[2], D3D12_SHADER_VISIBILITY_VERTEX);
	// 根签名描述
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	// 创建根签名
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
		IID_PPV_ARGS(&mRootSignature)));

	// 创建 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputDesc, _countof(inputDesc) };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS.pShaderBytecode = vs->GetBufferPointer();
	psoDesc.VS.BytecodeLength = vs->GetBufferSize();
	psoDesc.PS.pShaderBytecode = ps->GetBufferPointer();
	psoDesc.PS.BytecodeLength = ps->GetBufferSize();
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSORGBA32)));

	mViewport = { 0.0f, 0.0f
		, static_cast<float>(AppConfig::ClientWidth), static_cast<float>(AppConfig::ClientHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	mScissorRect = { 0, 0
		, static_cast<LONG>(AppConfig::ClientWidth), static_cast<LONG>(AppConfig::ClientHeight) };

	// 创建 Texture Resource
	auto commandAllocator = mCommandAllocator[0];
	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(mCommandList->Reset(commandAllocator.Get(), nullptr));
	mTexture = TextureMgr::LoadTexture2DFromFile("F:\\OpenLight\\timg.jpg",
		mDevice,
		mCommandList,
		mCommandQueue);
	//	ThrowIfFailed(mCommandList->Close());

	// 创建 Texture SRV
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		mSRVIndex = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mDescriptorHeap->AddSRV(&mSRVIndex, mTexture.Get(), srvDesc);
	}

	// 创建 CBV
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mCBTrans->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = PAD(sizeof(CBTrans), 256);

		mCBVIndex = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mDescriptorHeap->AddCBV(&mCBVIndex, mCBTrans.Get(), cbvDesc);
	}


	// 创建 Sampler
	

	D3D12_SAMPLER_DESC samplerDesc = {  };
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MinLOD = 0.f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// Sampler 1
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	mSamplerIndices[0] = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	mDescriptorHeap->AddSampler(&mSamplerIndices[0], samplerDesc);
	// Sampler 2
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	mSamplerIndices[1] = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	mDescriptorHeap->AddSampler(&mSamplerIndices[1], samplerDesc);

	// Sampler 3
	
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	mSamplerIndices[2] = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	mDescriptorHeap->AddSampler(&mSamplerIndices[2], samplerDesc);


	InitPostprocess();
	InitSkyBox();

}

void CubeRenderer::Render()
{
	Update();

	auto commandAllocator = mCommandAllocator[mCurrentBackBufferIndex];
	auto backBuffer = mBackBuffer[mCurrentBackBufferIndex];
	
	commandAllocator->Reset();
	mCommandList->Reset(commandAllocator.Get(), nullptr);

	// 切换 Scene Color 状态	
	if(mGammaCorrect)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = mSceneColorBuffer[mCurrentBackBufferIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		mCommandList->ResourceBarrier(1, &barrier);
	}


	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// 切换 后备RTV 的状态 
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1, &barrier);


	
	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	D3D12_CPU_DESCRIPTOR_HANDLE rtv;
	if (mGammaCorrect)
	{
		rtv = mPostprocessRTVIndex[mCurrentBackBufferIndex].cpuHandle;
	}
	else
	{
		rtv = (mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		rtv.ptr += mCurrentBackBufferIndex * mRTVDescriptorSize;
	}
	//设置渲染目标
	mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

	mCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);


	RenderSkyBox();

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	if (mGammaCorrect)
		mCommandList->SetPipelineState(mPSORGBA32.Get());
	else
		mCommandList->SetPipelineState(mPSO.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
		mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// 设置SRV
	mCommandList->SetGraphicsRootDescriptorTable(0, mDescriptorHeap->GPUHandle(mSRVIndex));

	// 设置 Sampler
	
	mCommandList->SetGraphicsRootDescriptorTable(1, mDescriptorHeap->GPUHandle(mSamplerIndices[mSamplerIndex]));

	// 设置 CBV
	mCommandList->SetGraphicsRootDescriptorTable(2, mDescriptorHeap->GPUHandle(mCBVIndex));

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mCubeMeshVBView);
	mCommandList->IASetIndexBuffer(&mCubeMeshIBView);
	//Draw Call！！！
	mCommandList->DrawIndexedInstanced(mCubeMesh->submeshs[0].indices.size(), 1, 0, 0, 0);

	// 如果有后处理
	if (mGammaCorrect)
	{
		// 切换 Scene Color
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = mSceneColorBuffer[mCurrentBackBufferIndex].Get();
		barrier.Transition.StateBefore =  D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter =D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		mCommandList->ResourceBarrier(1, &barrier);

		mCommandList->SetGraphicsRootSignature(mPostprocessSignature.Get());
		mCommandList->SetPipelineState(mPostprocessPSO.Get());
		ID3D12DescriptorHeap* ppHeaps[] = { mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		mCommandList->SetGraphicsRootDescriptorTable(0, mDescriptorHeap->GPUHandle(mPostprocessSRVIndex[mCurrentBackBufferIndex]));
		rtv = (mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		rtv.ptr += mCurrentBackBufferIndex * mRTVDescriptorSize;
		mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

		mCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
		mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mCommandList->IASetVertexBuffers(0, 1, &mQuadVBView);
		mCommandList->IASetIndexBuffer(&mQuadIBView);
		//Draw Call！！！
		mCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	}


	// Present
	barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1, &barrier);

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* const commandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	UINT syncInterval = AppConfig::VSync ? 1 : 0;
	UINT presentFlags = AppConfig::TearingSupported && !AppConfig::VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(mSwapChain->Present(syncInterval, presentFlags));

	mFrameFenceValues[mCurrentBackBufferIndex] = Signal(mCommandQueue, mFence, mFenceValue);

	mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	WaitForFenceValue(mFence.Get(), mFrameFenceValues[mCurrentBackBufferIndex], mFenceEvent);
}

void CubeRenderer::InitSkyBox()
{
	mSkyBoxMesh = ObjMeshLoader::loadObjMeshFromFile("F:\\OpenLight\\Sphere.obj");
	auto commandAllocator = mCommandAllocator[0];
	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(mCommandList->Reset(commandAllocator.Get(), nullptr));
	mSkyBoxEnvMap = TextureMgr::LoadTexture2DFromFile("F:\\OpenLight\\HDR_029_Sky_Cloudy_Ref.exr",
		mDevice,
		mCommandList,
		mCommandQueue);

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(StandardVertex) * mSkyBoxMesh->submeshs[0].vertices.size()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mSkyBoxVB)));
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * mSkyBoxMesh->submeshs[0].indices.size()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mSkyBoxIB)));

	{
		void* p = nullptr;
		ThrowIfFailed(mSkyBoxVB->Map(0, nullptr, &p));
		std::memcpy(p, &mSkyBoxMesh->submeshs[0].vertices[0], sizeof(StandardVertex) * mSkyBoxMesh->submeshs[0].vertices.size());
		mSkyBoxVB->Unmap(0,nullptr);
		ThrowIfFailed(mSkyBoxIB->Map(0, nullptr, &p));
		std::memcpy(p, &mSkyBoxMesh->submeshs[0].indices[0], sizeof(UINT) * mSkyBoxMesh->submeshs[0].indices.size());
		mSkyBoxIB->Unmap(0,nullptr);
	}
	mSkyBoxVBView.BufferLocation = mSkyBoxVB->GetGPUVirtualAddress();
	mSkyBoxVBView.StrideInBytes = sizeof(StandardVertex);
	mSkyBoxVBView.SizeInBytes = sizeof(StandardVertex) * mSkyBoxMesh->submeshs[0].vertices.size();
	mSkyBoxIBView.BufferLocation = mSkyBoxIB->GetGPUVirtualAddress();
	mSkyBoxIBView.Format = DXGI_FORMAT_R32_UINT;
	mSkyBoxIBView.SizeInBytes = sizeof(UINT) * mSkyBoxMesh->submeshs[0].indices.size();

	// 创建 InputLayout
	D3D12_INPUT_ELEMENT_DESC inputDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	WRL::ComPtr<ID3DBlob> vs;
	WRL::ComPtr<ID3DBlob> ps;

#if defined(_DEBUG)		
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\SkyBoxVS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "SkyBoxMainVS", "vs_5_0", compileFlags, 0, &vs, nullptr));
	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\SkyBoxPS.hlsl",
		nullptr, D3D_HLSL_DEFUALT_INCLUDE, "SkyBoxMainPS", "ps_5_0", compileFlags, 0, &ps, nullptr));

	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	CD3DX12_DESCRIPTOR_RANGE1 descRange[2];
	descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
	descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0);
	rootParameters[0].InitAsDescriptorTable(1, &descRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsDescriptorTable(1, &descRange[1], D3D12_SHADER_VISIBILITY_ALL);
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
	rootSignatureDesc.Init_1_1(2, rootParameters,
		1, &samplerDesc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
		IID_PPV_ARGS(&mSkyBoxSignature)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputDesc, _countof(inputDesc) };
	psoDesc.pRootSignature = mSkyBoxSignature.Get();
	psoDesc.VS.pShaderBytecode = vs->GetBufferPointer();
	psoDesc.VS.BytecodeLength = vs->GetBufferSize();
	psoDesc.PS.pShaderBytecode = ps->GetBufferPointer();
	psoDesc.PS.BytecodeLength = ps->GetBufferSize();
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mSkyBoxPSO)));
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mSkyBoxPSORGB32)));
	

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(PADCB(sizeof(CBTrans))),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mSkyBoxCBTrans)));



	// 创建 Texture SRV
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		mSkyBoxSRVIndex = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mDescriptorHeap->AddSRV(&mSkyBoxSRVIndex, mSkyBoxEnvMap.Get(), srvDesc);
	}

	{
		
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mSkyBoxCBTrans->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = PADCB(sizeof(CBTrans));
		mSkyBoxCBVIndex = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mDescriptorHeap->AddCBV(&mSkyBoxCBVIndex, mSkyBoxCBTrans.Get(), cbvDesc);
		ThrowIfFailed(mSkyBoxCBTrans->Map(0, nullptr, (void**)&mSkyBoxCBTransGPUPtr));
	}

//	RegisterResource(mSkyBoxEnvMap, SRV);

}

void CubeRenderer::RenderSkyBox()
{
	mCommandList->SetGraphicsRootSignature(mSkyBoxSignature.Get());
	if (mGammaCorrect)
		mCommandList->SetPipelineState(mSkyBoxPSORGB32.Get());
	else
		mCommandList->SetPipelineState(mSkyBoxPSO.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { mDescriptorHeap->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)};
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// 设置SRV
	mCommandList->SetGraphicsRootDescriptorTable(0, mDescriptorHeap->GPUHandle(mSkyBoxSRVIndex));

	// 设置 CBV
	mCommandList->SetGraphicsRootDescriptorTable(1, mDescriptorHeap->GPUHandle(mSkyBoxCBVIndex));

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mSkyBoxVBView);
	mCommandList->IASetIndexBuffer(&mSkyBoxIBView);
	//Draw Call！！！
	mCommandList->DrawIndexedInstanced(mSkyBoxMesh->submeshs[0].indices.size(), 1, 0, 0, 0);

}

void CubeRenderer::Update()
{
	FPS();
	if (GetAsyncKeyState('K') & 0x8000)
	{
		mSamplerIndex = (++mSamplerIndex) % mSamplerCount;
		Sleep(100);
	}
	mTimer.tick();

	if (GetAsyncKeyState('P') & 0x8000)
	{
		mGammaCorrect = !mGammaCorrect;
		Sleep(100);
	}

	// 相机更新
	static float speed = 20.f;
	float dt = mTimer.deltaTime();
	if (GetAsyncKeyState('W') & 0x8000)
		mCamera->walk(speed*dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera->walk(-speed * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera->strafe(-speed * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera->strafe(speed*dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera->yaw(+5.f * dt);
	if (GetAsyncKeyState('E') & 0x8000)
		mCamera->yaw(-5.f * dt);


	mCamera->updateMatrix();

	XMMATRIX vp = mCamera->getViewProj();
	XMMATRIX world = XMMatrixIdentity();

	XMStoreFloat4x4(&mCBTransGPUPtr->wvp, XMMatrixTranspose(vp));
	XMStoreFloat4x4(&mCBTransGPUPtr->world, XMMatrixTranspose(world));
	XMStoreFloat4x4(&mCBTransGPUPtr->invTranspose, XMMatrixTranspose(world));

	world = XMMatrixScaling(10, 10, 10);
	XMStoreFloat4x4(&mSkyBoxCBTransGPUPtr->wvp, XMMatrixTranspose(world * vp));
	XMStoreFloat4x4(&mSkyBoxCBTransGPUPtr->world, XMMatrixTranspose(world));
	XMStoreFloat4x4(&mSkyBoxCBTransGPUPtr->invTranspose, XMMatrixTranspose(world));
}

struct QuadVertex
{
	XMFLOAT3 positionL;
	XMFLOAT2 texcoord;
};

void CubeRenderer::InitPostprocess()
{
	// 创建 InputLayout
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


	// 创建根签名
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

	// 创建 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputDesc, _countof(inputDesc) };
	psoDesc.pRootSignature = mPostprocessSignature.Get();
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
		void *p;
		ThrowIfFailed(mQuadVB->Map(0, nullptr, &p));
		std::memcpy(p, &vertices[0], sizeof(QuadVertex) * vertices.size());
		mQuadVB->Unmap(0,nullptr);
	
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

	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	std::memcpy(&d3dClearValue.Color, clearColor, sizeof(FLOAT) * 4);
	for (int i = 0; i < AppConfig::NumFrames; ++i)
	{
		ThrowIfFailed(mDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_R32G32B32A32_FLOAT, 
				AppConfig::ClientWidth, 
				AppConfig::ClientHeight,
				1,1,1,0,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			&d3dClearValue,
			IID_PPV_ARGS(&mSceneColorBuffer[i])));
	}

	{
		

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {  };
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		for (int i = 0; i < AppConfig::NumFrames; ++i)
		{
			
			mPostprocessSRVIndex[i] = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			mDescriptorHeap->AddSRV(&mPostprocessSRVIndex[i], mSceneColorBuffer[i].Get(), srvDesc);
			
			mPostprocessRTVIndex[i] = mDescriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			mDescriptorHeap->AddRTV(&mPostprocessRTVIndex[i], mSceneColorBuffer[i].Get(), rtvDesc);
		}
	}




	

}

void CubeRenderer::FPS()
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
