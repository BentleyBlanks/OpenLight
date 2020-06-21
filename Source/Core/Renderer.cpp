#include "Renderer.h"
#include "Utility.h"
#include <cassert>
#include "d3dx12.h"
#include <tchar.h>



struct Vertex
{
	DirectX::XMFLOAT4 position; // POSH
	DirectX::XMFLOAT4 color;    // COLOR
	Vertex() {}
	Vertex(const DirectX::XMFLOAT4& pos,
		const DirectX::XMFLOAT4& c) :position(pos), color(c) {}
};

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

void GetAllAdapter(bool useWarp,
	std::vector<IDXGIAdapter4*>& adapterList)
{
	
}

ID3D12Device5* CreateDevice(IDXGIAdapter4* adapter)
{
	DXGI_ADAPTER_DESC adapterDesc = {};
	adapter->GetDesc(&adapterDesc);
//	std::cout << "aaa" << std::endl;
	//ErrorBoxW(adapterDesc.Description);
	ID3D12Device5* d3d12Device5 = nullptr;
	ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device5)));

#ifdef _DEBUG
	ID3D12InfoQueue* pInfoQueue = nullptr;
	if (SUCCEEDED(d3d12Device5->QueryInterface(IID_PPV_ARGS(&pInfoQueue))))
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

	return d3d12Device5;
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
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue		   ,
													   hWnd				   ,
													   &swapChainDesc      ,
													   nullptr             ,
													   nullptr             ,
													   &swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature.
	// Switching to fullscreen will be handled manualy.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd , DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain4)));

	return dxgiSwapChain4;
}

ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device5* device , D3D12_DESCRIPTOR_HEAP_TYPE type , uint32_t numDescriptors)
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type           = type;
	ThrowIfFailed(device->CreateDescriptorHeap(&desc , IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

void UpdateRenderTargetViews(ID3D12Device5* device , IDXGISwapChain4* swapChain , ID3D12DescriptorHeap* descriptorHeap, ID3D12Resource* backBufferList [AppConfig::NumFrames])
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



ID3D12CommandAllocator* CreateCommandAllocator(ID3D12Device5* device , D3D12_COMMAND_LIST_TYPE type)
{
	ID3D12CommandAllocator* CommandAllocator = nullptr;
	ThrowIfFailed(device->CreateCommandAllocator(type , IID_PPV_ARGS(&CommandAllocator)));

	return CommandAllocator;
}

ID3D12GraphicsCommandList* CreateCommandList(ID3D12Device5* device , ID3D12CommandAllocator* commandAllocator , D3D12_COMMAND_LIST_TYPE type)
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

uint64_t Renderer::Signal(ID3D12CommandQueue* commandQueue , ID3D12Fence* Fence , uint64_t& FenceValue)
{
	uint64_t FenceValueForSignal = ++FenceValue;
	ThrowIfFailed(commandQueue->Signal(Fence , FenceValueForSignal));
	return FenceValueForSignal;
}

void Renderer::WaitForFenceValue(ID3D12Fence* Fence ,
					   uint64_t FenceValue ,
					   HANDLE FenceHandle ,
					   std::chrono::milliseconds duration )
{
	if (Fence->GetCompletedValue() < FenceValue)
	{
		ThrowIfFailed(Fence->SetEventOnCompletion(FenceValue , FenceHandle));
		::WaitForSingleObject(FenceHandle , static_cast<DWORD>(duration.count()));
	}
}

void Renderer::Flush(ID3D12CommandQueue* commandQueue , ID3D12Fence* Fence , uint64_t& FenceValue , HANDLE FenceEvent)
{
	uint64_t FenceValueForSignal = Signal(commandQueue , Fence , FenceValue);
	WaitForFenceValue(Fence , FenceValueForSignal , FenceEvent);
}

void Renderer::Flush(ID3D12CommandQueue * commandQueue, ID3D12Fence * Fence, uint64_t & FenceValue)
{
	HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
	uint64_t FenceValueForSignal = Signal(commandQueue, Fence, FenceValue);
	WaitForFenceValue(Fence, FenceValueForSignal, eventHandle);
	CloseHandle(eventHandle);
}

void Renderer::renderGUI()
{
	mCommandList->SetDescriptorHeaps(1, &mPd3dSrvDescHeap);
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList);
}



Renderer::Renderer()
{
}

Renderer::~Renderer()
{
	ReleaseCom(mDevice);
	ReleaseCom(mCommandQueue);
	ReleaseCom(mSwapChain);
	ReleaseCom(mCommandList);
	ReleaseCom(mRTVDescriptorHeap);
	for (int i = 0; i < AppConfig::NumFrames; ++i)
	{
		ReleaseCom(mBackBuffer[i]);
		ReleaseCom(mCommandAllocator[i]);
	}	
	ReleaseCom(mPd3dSrvDescHeap);
	ReleaseCom(mFence);
}

void Renderer::Init(HWND hWnd)
{
	mHWND = hWnd;
	EnableDebugLayer();

	AppConfig::TearingSupported = CheckTearingSupport();

	IDXGIAdapter4* dxgiAdapter4 = GetAdapter(AppConfig::UseWarp);
	mDevice = CreateDevice(dxgiAdapter4);

	mCommandQueue = CreateCommandQueue(mDevice , D3D12_COMMAND_LIST_TYPE_DIRECT);
	mSwapChain	 = CreateSwapChain(hWnd , mCommandQueue , AppConfig::ClientWidth , AppConfig::ClientHeight , AppConfig::NumFrames);

	mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

	mRTVDescriptorHeap = CreateDescriptorHeap(mDevice , D3D12_DESCRIPTOR_HEAP_TYPE_RTV, AppConfig::NumFrames);
	mRTVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	mDSVDescriptorHeap = CreateDescriptorHeap(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
	mDSVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	UpdateRenderTargetViews(mDevice , mSwapChain , mRTVDescriptorHeap , mBackBuffer);
	

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			AppConfig::ClientWidth,
			AppConfig::ClientHeight,
			1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optClear,
		IID_PPV_ARGS(&mDepthBuffer)));
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	mDevice->CreateDepthStencilView(mDepthBuffer, &dsvDesc, mDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());



	for (int i = 0; i < AppConfig::NumFrames; i++)
	{
		mCommandAllocator[i] = CreateCommandAllocator(mDevice , D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	mCommandList = CreateCommandList(mDevice , mCommandAllocator[mCurrentBackBufferIndex] , D3D12_COMMAND_LIST_TYPE_DIRECT);

	mFence = CreateFence(mDevice);
	mFenceEvent = CreateEventHandle();
	

	// 初始化 IMGUI
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mPd3dSrvDescHeap)) != S_OK)
			assert(false);
	}


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX12_Init(mDevice, AppConfig::NumFrames,
		DXGI_FORMAT_R8G8B8A8_UNORM, mPd3dSrvDescHeap,
		mPd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		mPd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());


}

void Renderer::Render()
{
	auto commandAllocator = mCommandAllocator[mCurrentBackBufferIndex];
	auto backBuffer       = mBackBuffer[mCurrentBackBufferIndex];

	commandAllocator->Reset();
	mCommandList->Reset(commandAllocator, nullptr);



	// Clear RenderTarget
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = backBuffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1 , &barrier);

	FLOAT clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
	D3D12_CPU_DESCRIPTOR_HANDLE rtv(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.ptr += mCurrentBackBufferIndex * mRTVDescriptorSize;

	//设置渲染目标
	mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

	mCommandList->ClearRenderTargetView(rtv , clearColor , 0 , nullptr);



	// Present
	barrier = {};
	barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = backBuffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	mCommandList->ResourceBarrier(1 , &barrier);

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* const commandLists[] = {mCommandList};
	mCommandQueue->ExecuteCommandLists(_countof(commandLists) , commandLists);

	UINT syncInterval = AppConfig::VSync ? 1 : 0;
	UINT presentFlags = AppConfig::TearingSupported && !AppConfig::VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(mSwapChain->Present(syncInterval , presentFlags));

	mFrameFenceValues[mCurrentBackBufferIndex] = Signal(mCommandQueue , mFence , mFenceValue);

	mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	WaitForFenceValue(mFence , mFrameFenceValues[mCurrentBackBufferIndex] , mFenceEvent);
}

void Renderer::Resize(uint32_t width , uint32_t height)
{
	if (AppConfig::ClientWidth != width || AppConfig::ClientHeight != height)
	{
		AppConfig::ClientWidth  = std::max(1u , width);
		AppConfig::ClientHeight = std::max(1u , height);

		Flush(mCommandQueue , mFence , mFenceValue , mFenceEvent);

		for (int i = 0; i < AppConfig::NumFrames; i++)
		{
			mBackBuffer[i]->Release();
			mFrameFenceValues[i] = mFrameFenceValues[mCurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(mSwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(mSwapChain->ResizeBuffers(AppConfig::NumFrames , width , height , swapChainDesc.BufferDesc.Format , swapChainDesc.Flags));

		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

		UpdateRenderTargetViews(mDevice , mSwapChain , mRTVDescriptorHeap , mBackBuffer);
	}
}

