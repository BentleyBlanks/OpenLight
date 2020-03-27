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
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));

	mViewport = { 0.0f, 0.0f
		, static_cast<float>(AppConfig::ClientWidth), static_cast<float>(AppConfig::ClientHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	mScissorRect = { 0, 0
		, static_cast<LONG>(AppConfig::ClientWidth), static_cast<LONG>(AppConfig::ClientHeight) };

	// 创建 SRV CBV Heap
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 2;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSRVCBVHeap)));

	// 创建 Sampler Heap
	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
	samplerHeapDesc.NumDescriptors = mSamplerCount;
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&mSamplerHeap)));



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
		mDevice->CreateShaderResourceView(mTexture.Get(), &srvDesc, mSRVCBVHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// 创建 CBV
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mCBTrans->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = PAD(sizeof(CBTrans),256);
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSRVCBVHeap->GetCPUDescriptorHandleForHeapStart(),
			1,
			mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

		mDevice->CreateConstantBufferView(&cbvDesc, handle);
	}


	// 创建 Sampler
	auto samplerDescriptorOffset = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle(mSamplerHeap->GetCPUDescriptorHandleForHeapStart());

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
	mDevice->CreateSampler(&samplerDesc, samplerHandle);

	// Sampler 2
	samplerHandle.Offset(samplerDescriptorOffset);
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	mDevice->CreateSampler(&samplerDesc, samplerHandle);

	// Sampler 3
	samplerHandle.Offset(samplerDescriptorOffset);
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	mDevice->CreateSampler(&samplerDesc, samplerHandle);


}

void CubeRenderer::Render()
{
	Update();

	auto commandAllocator = mCommandAllocator[mCurrentBackBufferIndex];
	auto backBuffer = mBackBuffer[mCurrentBackBufferIndex];

	commandAllocator->Reset();
	mCommandList->Reset(commandAllocator.Get(), nullptr);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	mCommandList->SetPipelineState(mPSO.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { mSRVCBVHeap.Get(),mSamplerHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// 设置SRV
	mCommandList->SetGraphicsRootDescriptorTable(0, mSRVCBVHeap->GetGPUDescriptorHandleForHeapStart());
	
	// 设置 Sampler
	CD3DX12_GPU_DESCRIPTOR_HANDLE samplerHandle(mSamplerHeap->GetGPUDescriptorHandleForHeapStart(),
		mSamplerIndex,
		mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));
	mCommandList->SetGraphicsRootDescriptorTable(1, samplerHandle);
	
	// 设置 CBV
	mCommandList->SetGraphicsRootDescriptorTable(2,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mSRVCBVHeap->GetGPUDescriptorHandleForHeapStart(),
			1,
			mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));



	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Clear RenderTarget
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1, &barrier);

	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	D3D12_CPU_DESCRIPTOR_HANDLE rtv(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.ptr += mCurrentBackBufferIndex * mRTVDescriptorSize;

	//设置渲染目标
	mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

	mCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mCubeMeshVBView);
	mCommandList->IASetIndexBuffer(&mCubeMeshIBView);
	//Draw Call！！！
	mCommandList->DrawIndexedInstanced(mCubeMesh->submeshs[0].indices.size(), 1, 0, 0, 0);



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

void CubeRenderer::Update()
{

	if (GetAsyncKeyState('K') & 0x8000)
	{
		mSamplerIndex = (++mSamplerIndex) % mSamplerCount;
		Sleep(100);
	}
	mTimer.tick();


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
}
