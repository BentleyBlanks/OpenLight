#pragma once
#include"Renderer.h"



class TextureCubRenderer:public Renderer
{
public:
	TextureCubRenderer();
	virtual void Init(HWND hWnd) override;
	virtual void Render() override;
	
protected:
	WRL::ComPtr<ID3D12Resource1>		mTriVB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW			mTriVBView;
	WRL::ComPtr<ID3D12Resource1>		mTriIB = nullptr;
	D3D12_INDEX_BUFFER_VIEW				mTriIBView;
	WRL::ComPtr<ID3D12Resource1>		mTexture = nullptr;
	WRL::ComPtr<ID3D12DescriptorHeap>	mSRVHeap = nullptr;
	WRL::ComPtr<ID3D12PipelineState>	mPSO = nullptr;
	WRL::ComPtr<ID3D12RootSignature>	mRootSignature = nullptr;
	D3D12_VIEWPORT						mViewport;
	D3D12_RECT							mScissorRect;
};