#include"TextureCubeRenderer.h"
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
struct Vertex
{
	XMFLOAT4 position;
	XMFLOAT2 texcoord;
	Vertex() {}
	Vertex(const DirectX::XMFLOAT4& pos,
		const DirectX::XMFLOAT2& t) :position(pos), texcoord(t) {}
};
TextureCubRenderer::TextureCubRenderer()
{
}



void TextureCubRenderer::Init(HWND hWnd)
{
	Renderer::Init(hWnd);
	
	// 创建 InputLayout
	D3D12_INPUT_ELEMENT_DESC inputDesc[]=
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// 创建顶点
	Vertex triangleVertices[4] = {
	Vertex(DirectX::XMFLOAT4(-0.5f,-0.5f,1.f,1.f),DirectX::XMFLOAT2(0.f,0.f)),
	Vertex(DirectX::XMFLOAT4(-0.5f,0.5f,1.f,1.f),DirectX::XMFLOAT2(0.f,1.f)),
	Vertex(DirectX::XMFLOAT4(0.5f,0.5f,1.f,1.f),DirectX::XMFLOAT2(1.f,1.f)), 
	Vertex(DirectX::XMFLOAT4(0.5f,-0.5f,1.f,1.f),DirectX::XMFLOAT2(1.f,0.f))};

	UINT indices[6] = { 0,1,2 , 2,3,0 };
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(triangleVertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mTriVB)));

	UINT8* pVertices = nullptr;
	ThrowIfFailed(mTriVB->Map(0, nullptr, reinterpret_cast<void**>(&pVertices)));
	std::memcpy(pVertices, triangleVertices, sizeof(triangleVertices));
	mTriVB->Unmap(0, nullptr);

	mTriVBView.BufferLocation = mTriVB->GetGPUVirtualAddress();
	mTriVBView.StrideInBytes = sizeof(Vertex);
	mTriVBView.SizeInBytes = sizeof(triangleVertices);

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mTriIB)));

	UINT* pIndices = nullptr;
	ThrowIfFailed(mTriIB->Map(0, nullptr, reinterpret_cast<void**>(&pIndices)));
	std::memcpy(pIndices, indices, sizeof(indices));
	mTriIB->Unmap(0, nullptr);

	mTriIBView.BufferLocation = mTriIB->GetGPUVirtualAddress();
	mTriIBView.Format = DXGI_FORMAT_R32_UINT;
	mTriIBView.SizeInBytes = sizeof(indices);


	// 创建 Shader
	WRL::ComPtr<ID3DBlob> vs = nullptr;
	WRL::ComPtr<ID3DBlob> ps = nullptr;

#if defined(_DEBUG)		
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif 

	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\TextureQuadVS.hlsl",
		nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vs, nullptr));


	ThrowIfFailed(D3DCompileFromFile(L"F:\\OpenLight\\Shader\\TextureQuadPS.hlsl",
		nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &ps, nullptr));

	
	// 创建根签名参数
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	CD3DX12_DESCRIPTOR_RANGE1 descRange;
	descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
	rootParameters[0].InitAsDescriptorTable(1, &descRange,D3D12_SHADER_VISIBILITY_PIXEL);
	// 采样器
	CD3DX12_STATIC_SAMPLER_DESC sampleDesc(0);
	sampleDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	// 根签名描述
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(1, rootParameters,
		1, &sampleDesc,
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

	// 创建 SRV Heap
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSRVHeap)));


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
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	mDevice->CreateShaderResourceView(mTexture.Get(), &srvDesc, mSRVHeap->GetCPUDescriptorHandleForHeapStart());

}

void TextureCubRenderer::Render()
{
	auto commandAllocator = mCommandAllocator[mCurrentBackBufferIndex];
	auto backBuffer = mBackBuffer[mCurrentBackBufferIndex];

	commandAllocator->Reset();
	mCommandList->Reset(commandAllocator.Get(), nullptr);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	mCommandList->SetPipelineState(mPSO.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { mSRVHeap.Get() };
	mCommandList->SetDescriptorHeaps(1, ppHeaps);
	mCommandList->SetGraphicsRootDescriptorTable(0, mSRVHeap->GetGPUDescriptorHandleForHeapStart());
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
	mCommandList->IASetVertexBuffers(0, 1, &mTriVBView);
	mCommandList->IASetIndexBuffer(&mTriIBView);
	//Draw Call！！！
	mCommandList->DrawIndexedInstanced(6,1,0,0,0);



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
