#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "AppConfig.h"
using namespace Microsoft;
struct Vertex
{
	DirectX::XMFLOAT4 position; // POSH
	DirectX::XMFLOAT4 color;    // COLOR
	Vertex() {}
	Vertex(const DirectX::XMFLOAT4& pos,
		const DirectX::XMFLOAT4& c) :position(pos), color(c) {}
};

class Renderer
{
public:
	Renderer();

	void Init(HWND hWnd);
	void Render();
	void Resize(uint32_t width , uint32_t height);

private:
	void InitTriangle();

	ID3D12Device2*             Device       = nullptr;
	ID3D12CommandQueue*        CommandQueue = nullptr;
	IDXGISwapChain4*           SwapChain    = nullptr;
	ID3D12GraphicsCommandList* CommandList  = nullptr;
	ID3D12DescriptorHeap*	   RTVDescriptorHeap = nullptr;;
	ID3D12Resource*            BackBuffer[AppConfig::NumFrames];
	ID3D12CommandAllocator*	   CommandAllocator[AppConfig::NumFrames];

	uint32_t RTVDescriptorSize;
	uint32_t CurrentBackBufferIndex;

	// Triangle Test
	WRL::ComPtr<ID3D12Resource> triangleVB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	triangleVBView;
	WRL::ComPtr<ID3D12PipelineState> pso = nullptr;
	WRL::ComPtr<ID3D12RootSignature> signature = nullptr;
	D3D12_VIEWPORT				viewport;
	D3D12_RECT					scissorRect;

	// Synchronization Objects
	ID3D12Fence* Fence = nullptr;
	uint64_t	 FenceValue = 0;
	uint64_t	 FrameFenceValues[AppConfig::NumFrames] = {};
	HANDLE		 FenceEvent;
};