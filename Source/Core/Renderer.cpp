#include "Renderer.h"
#include <cassert>
#include <chrono>
#include <d3dx12.h>
#include "Utility.h"
#include "Scene.h"
#include "GameObject.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

void EnableDebugLayer()
{
#ifdef _DEBUG
	ID3D12Debug* debugInterface = nullptr;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

IDXGIAdapter4* GetAdapter(bool useWarp)
{
	IDXGIFactory4* dxgiFactory = nullptr;
	UINT createFactoryFlags = 0;
#ifdef _DEBUG
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags , IID_PPV_ARGS(&dxgiFactory)));

	IDXGIAdapter1* dxgiAdapter1 = nullptr;
	IDXGIAdapter4* dxgiAdapter4 = nullptr;

	if (useWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		dxgiAdapter4 = dynamic_cast<IDXGIAdapter4*>(dxgiAdapter1);
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i , &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
			{
				if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter1 , D3D_FEATURE_LEVEL_11_0 , __uuidof(ID3D12Device) , nullptr)))
				{
					if (dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
					{
						maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
						ThrowIfFailed(dxgiAdapter1->QueryInterface(IID_PPV_ARGS(&dxgiAdapter4)));
					}
				}
			}
		}
	}

	return dxgiAdapter4;
}

ID3D12Device2* CreateDevice(IDXGIAdapter4* adapter)
{
	ID3D12Device2* d3d12Device2 = nullptr;
	ThrowIfFailed(D3D12CreateDevice(adapter , D3D_FEATURE_LEVEL_11_0 , IID_PPV_ARGS(&d3d12Device2)));

#ifdef _DEBUG
	ID3D12InfoQueue* pInfoQueue = nullptr;
	if (SUCCEEDED(d3d12Device2->QueryInterface(IID_PPV_ARGS(&pInfoQueue))))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION , TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR      , TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING    , TRUE);

		// Suppress message based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_MESSAGE_ID DenyIds[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs        = _countof(DenyIds);
		NewFilter.DenyList.pIDList       = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	return d3d12Device2;
}

ID3D12CommandQueue* CreateCommandQueue(ID3D12Device2* device , D3D12_COMMAND_LIST_TYPE type)
{
	ID3D12CommandQueue* d3d12CommandQueue = nullptr;
	
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type     = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&desc , IID_PPV_ARGS(&d3d12CommandQueue)));
	return d3d12CommandQueue;
}

// variable rate display require tearing to be enable in the directx 12 application to function correctly.
bool CheckTearingSupport()
{
	BOOL allowTearing = FALSE;

	IDXGIFactory4* factory4 = nullptr;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		IDXGIFactory5* factory5 = nullptr;
		if (SUCCEEDED(factory4->QueryInterface(IID_PPV_ARGS(&factory5))))
		{
			if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING ,
													 &allowTearing                      ,
													 sizeof(allowTearing))))
			{
				allowTearing = FALSE;
			}
		}
	}

	return allowTearing == TRUE;
}

IDXGISwapChain4* CreateSwapChain(HWND hWnd , ID3D12CommandQueue* commandQueue , uint32_t width , uint32_t height , uint32_t bufferCount)
{
	IDXGISwapChain4* dxgiSwapChain4 = nullptr;
	IDXGIFactory4*   dxgiFactory4 = nullptr;

	UINT createFactoryFlags = 0;
#ifdef _DEBUG
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags , IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width                 = width;
	swapChainDesc.Height                = height;
	swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo                = FALSE;
	swapChainDesc.SampleDesc            = {1, 0};
	swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount           = bufferCount;
	swapChainDesc.Scaling               = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags                 = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	IDXGISwapChain1* swapChain1 = nullptr;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue  ,
													   hWnd          ,
													   &swapChainDesc,
													   nullptr       ,
													   nullptr       ,
													   &swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature.
	// Switching to fullscreen will be handled manualy.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd , DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain4)));

	return dxgiSwapChain4;
}

ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device2* device , D3D12_DESCRIPTOR_HEAP_TYPE type , uint32_t numDescriptors)
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type           = type;
	ThrowIfFailed(device->CreateDescriptorHeap(&desc , IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

void UpdateRenderTargetViews(ID3D12Device2* device , IDXGISwapChain4* swapChain , ID3D12DescriptorHeap* descriptorHeap, ID3D12Resource** backBufferList)
{
	auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < AppConfig::NumFrames; i++)
	{
		ID3D12Resource* backBuffer = nullptr;
		ThrowIfFailed(swapChain->GetBuffer(i , IID_PPV_ARGS(&backBuffer)));

		device->CreateRenderTargetView(backBuffer , nullptr , rtvHandle);
		
		backBufferList[i] = backBuffer;

		rtvHandle.ptr += rtvDescriptorSize;
	}
}

ID3D12CommandAllocator* CreateCommandAllocator(ID3D12Device* device , D3D12_COMMAND_LIST_TYPE type)
{
	ID3D12CommandAllocator* CommandAllocator = nullptr;
	ThrowIfFailed(device->CreateCommandAllocator(type , IID_PPV_ARGS(&CommandAllocator)));

	return CommandAllocator;
}

ID3D12GraphicsCommandList* CreateCommandList(ID3D12Device2* device , ID3D12CommandAllocator* commandAllocator , D3D12_COMMAND_LIST_TYPE type)
{
	ID3D12GraphicsCommandList* commandList = nullptr;
	ThrowIfFailed(device->CreateCommandList(0 , type , commandAllocator , nullptr , IID_PPV_ARGS(&commandList)));

	ThrowIfFailed(commandList->Close());

	return commandList;
}

ID3D12Fence* CreateFence(ID3D12Device* device)
{
	ID3D12Fence* Fence = nullptr;
	ThrowIfFailed(device->CreateFence(0 , D3D12_FENCE_FLAG_NONE , IID_PPV_ARGS(&Fence)));

	return Fence;
}

HANDLE CreateEventHandle()
{
	HANDLE FenceEvent = ::CreateEvent(NULL , FALSE , FALSE , NULL);
	assert(FenceEvent && "Failed to Create Fence Event");

	return FenceEvent;
}

uint64_t Signal(ID3D12CommandQueue* commandQueue , ID3D12Fence* Fence , uint64_t& FenceValue)
{
	uint64_t FenceValueForSignal = ++FenceValue;
	ThrowIfFailed(commandQueue->Signal(Fence , FenceValueForSignal));
	return FenceValueForSignal;
}

void WaitForFenceValue(ID3D12Fence* Fence ,
					   uint64_t FenceValue ,
					   HANDLE FenceHandle ,
					   std::chrono::milliseconds duration = std::chrono::milliseconds::max())
{
	if (Fence->GetCompletedValue() < FenceValue)
	{
		ThrowIfFailed(Fence->SetEventOnCompletion(FenceValue , FenceHandle));
		::WaitForSingleObject(FenceHandle , static_cast<DWORD>(duration.count()));
	}
}

void Flush(ID3D12CommandQueue* commandQueue , ID3D12Fence* Fence , uint64_t& FenceValue , HANDLE FenceEvent)
{
	uint64_t FenceValueForSignal = Signal(commandQueue , Fence , FenceValue);
	WaitForFenceValue(Fence , FenceValueForSignal , FenceEvent);
}

Renderer::Renderer()
{
	mScissorRect = D3D12_RECT{0 , 0 , LONG_MAX , LONG_MAX};
	mViewport    = D3D12_VIEWPORT{0.0f, 
								  0.0f, 
								  static_cast<float>(AppConfig::ClientWidth), 
								  static_cast<float>(AppConfig::ClientHeight),
								  D3D12_MIN_DEPTH, 
								  D3D12_MAX_DEPTH};
}

void Renderer::Init(HWND hWnd, Scene* scene)
{
	mScene = scene;

	EnableDebugLayer();

	AppConfig::TearingSupported = CheckTearingSupport();

	IDXGIAdapter4* dxgiAdapter4 = GetAdapter(AppConfig::UseWarp);
	mDevice = CreateDevice(dxgiAdapter4);

	mDirectQueue  = new CommandQueue(mDevice , D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeQueue = new CommandQueue(mDevice , D3D12_COMMAND_LIST_TYPE_COMPUTE);
	mCopyQueue    = new CommandQueue(mDevice , D3D12_COMMAND_LIST_TYPE_COPY);

	RTVDescriptorHeap = CreateDescriptorHeap(mDevice , D3D12_DESCRIPTOR_HEAP_TYPE_RTV , AppConfig::NumFrames);
	RTVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	SwapChain = CreateSwapChain(hWnd , mDirectQueue->GetD3D12CommandQueue() , AppConfig::ClientWidth , AppConfig::ClientHeight , AppConfig::NumFrames);
	mCurrentBackBufferIndex = 0;
	UpdateRenderTargetViews(mDevice , SwapChain , RTVDescriptorHeap , BackBuffer);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&dsvHeapDesc , IID_PPV_ARGS(&mDSVHeap)));

	ResizeDepthBuffer(AppConfig::ClientWidth , AppConfig::ClientHeight);

	{
		float angle = 0.0f;
		const DirectX::XMVECTOR rotateAxis = DirectX::XMVectorSet(0 , 1 , 1 , 0);
		mModelMatrix = DirectX::XMMatrixRotationAxis(rotateAxis , DirectX::XMConvertToRadians(angle));

		const DirectX::XMVECTOR eyePosition = DirectX::XMVectorSet(0 , 0 , -10 , 1);
		const DirectX::XMVECTOR focusPoint  = DirectX::XMVectorSet(0 , 0 , 0 , 1);
		const DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(0 , 1 , 0 , 0);
		mViewMatrix = DirectX::XMMatrixLookAtLH(eyePosition , focusPoint , upDirection);

		mFov = 45.0f;
		float aspectRatio = AppConfig::ClientWidth / static_cast<float>(AppConfig::ClientHeight);
		mProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(mFov) , aspectRatio , 0.1f , 100.0f);
	}
}

void Renderer::Render()
{
	auto CommandList = mDirectQueue->GetCommandList();
	
	auto BackBuffer = GetCurrentBackBuffer();
	auto RTV = GetCurrentRTV();
	auto DSV = GetCurrentDSV();

	{
		TransitionResource(CommandList , BackBuffer , D3D12_RESOURCE_STATE_PRESENT , D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT ClearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
		ClearRTV(CommandList , RTV , ClearColor);
		ClearDepth(CommandList , DSV);
	}

	for (int i = 0; i < mScene->GetNumObject(); i++)
	{
		auto ob     = mScene->GetVisibiltyObject(i);
		auto shader = ob->GetShader();

		CommandList->SetPipelineState(shader->GetPipelineState());
		CommandList->SetGraphicsRootSignature(shader->GetRootSignature());
		CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CommandList->IASetVertexBuffers(0 , 1 , &(ob->GetVertexBufferView()));
		CommandList->IASetIndexBuffer(&(ob->GetIndexBufferView()));
		CommandList->RSSetViewports(1 , &mViewport);
		CommandList->RSSetScissorRects(1 , &mScissorRect);
		CommandList->OMSetRenderTargets(1 , &RTV , false , &DSV);

		DirectX::XMMATRIX mvpMatrix = DirectX::XMMatrixMultiply(mModelMatrix , mViewMatrix);;
		mvpMatrix = DirectX::XMMatrixMultiply(mvpMatrix , mProjectionMatrix);
		CommandList->SetGraphicsRoot32BitConstants(0 , sizeof(DirectX::XMMATRIX) / 4 , &mvpMatrix , 0);

		CommandList->DrawIndexedInstanced(ob->GetNumIndices() , 1 , 0 , 0 , 0);
	}

	{
		TransitionResource(CommandList , BackBuffer , D3D12_RESOURCE_STATE_RENDER_TARGET , D3D12_RESOURCE_STATE_PRESENT);
		mFenceValues[mCurrentBackBufferIndex] = mDirectQueue->ExecuteCommandList(CommandList);

		mCurrentBackBufferIndex = Present();

		mDirectQueue->WaitForFenceValue(mFenceValues[mCurrentBackBufferIndex]);
	}
}

void Renderer::Resize(uint32_t width , uint32_t height)
{
	if (AppConfig::ClientWidth != width || AppConfig::ClientHeight != height)
	{
		AppConfig::ClientWidth  = std::max(1u , width);
		AppConfig::ClientHeight = std::max(1u , height);

		mViewport = D3D12_VIEWPORT{0, 0, static_cast<float>(width), static_cast<float>(height), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};

		ResizeDepthBuffer(width , height);

		// resize back buffer
		for (int i = 0; i < AppConfig::NumFrames; i++)
		{
			BackBuffer[i]->Release();
		}
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(SwapChain->ResizeBuffers(AppConfig::NumFrames , 
											   AppConfig::ClientWidth , 
											   AppConfig::ClientHeight , 
											   swapChainDesc.BufferDesc.Format , 
											   swapChainDesc.Flags));
		UpdateRenderTargetViews(mDevice , SwapChain , RTVDescriptorHeap , BackBuffer);
	}
}

void Renderer::Flush()
{
	if (mDirectQueue)	mDirectQueue->Flush();
	if (mComputeQueue)	mComputeQueue->Flush();
	if (mCopyQueue)		mCopyQueue->Flush();
}

ID3D12Device2* Renderer::GetDevice()
{
	return mDevice;
}

CommandQueue* Renderer::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return mDirectQueue;

	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return mComputeQueue;

	case D3D12_COMMAND_LIST_TYPE_COPY:
		return mComputeQueue;
	}

	return nullptr;
}

void Renderer::TransitionResource(ID3D12GraphicsCommandList2* CommandList , ID3D12Resource* Resource , D3D12_RESOURCE_STATES beForeState , D3D12_RESOURCE_STATES AfterState)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = Resource;
	barrier.Transition.StateBefore = beForeState;
	barrier.Transition.StateAfter  = AfterState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	CommandList->ResourceBarrier(1 , &barrier);
}

void Renderer::ClearRTV(ID3D12GraphicsCommandList2* CommandList , D3D12_CPU_DESCRIPTOR_HANDLE Rtv , FLOAT* ClearColor)
{
	CommandList->ClearRenderTargetView(Rtv , ClearColor , 0 , nullptr);
}

void Renderer::ClearDepth(ID3D12GraphicsCommandList2* CommandList , D3D12_CPU_DESCRIPTOR_HANDLE dsv , FLOAT depth)
{
	CommandList->ClearDepthStencilView(dsv , D3D12_CLEAR_FLAG_DEPTH , depth , 0 , 0 , nullptr);
}

void Renderer::ResizeDepthBuffer(int width , int height)
{
	// Flush any GPU commands might be referencing the depth buffer
	Flush();

	width  = std::max(1 , width);
	height = std::max(1 , height);

	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format       = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = {1.0f, 0};

	ThrowIfFailed(mDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT) ,
												  D3D12_HEAP_FLAG_NONE ,
												  &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT , width , height , 1 , 0 , 1 , 0 , D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) ,
												  D3D12_RESOURCE_STATE_DEPTH_WRITE ,
												  &optimizedClearValue ,
												  IID_PPV_ARGS(&mDepthBuffer)));

	D3D12_DEPTH_STENCIL_VIEW_DESC Dsv = {};
	Dsv.Format             = DXGI_FORMAT_D32_FLOAT;
	Dsv.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
	Dsv.Texture2D.MipSlice = 0;
	Dsv.Flags              = D3D12_DSV_FLAG_NONE;

	mDevice->CreateDepthStencilView(mDepthBuffer , &Dsv , mDSVHeap->GetCPUDescriptorHandleForHeapStart());
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::GetCurrentRTV() const
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.ptr += mCurrentBackBufferIndex * RTVDescriptorSize;
	return rtv;
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::GetCurrentDSV() const
{
	return mDSVHeap->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* Renderer::GetCurrentBackBuffer() const
{
	return BackBuffer[mCurrentBackBufferIndex];
}

UINT Renderer::Present()
{
	UINT syncInterval = AppConfig::VSync ? 1 : 0;
	UINT presentFlags = AppConfig::TearingSupported && !AppConfig::VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(SwapChain->Present(syncInterval , presentFlags));
	return SwapChain->GetCurrentBackBufferIndex();
}
