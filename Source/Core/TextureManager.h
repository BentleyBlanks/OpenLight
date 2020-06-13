#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "FreeImage.h"
using namespace Microsoft;
class TextureMgr
{
public:
	struct TextureInfo
	{
		// ͼƬ����ʵ���
		int width;
		// ͼƬ����ʵ�߶�
		int height;
		// �Դ��еĴ�С
		int memorySize;
	};
	static  WRL::ComPtr<ID3D12Resource1> LoadTexture2DFromFile(
		const std::string& fileName,
		WRL::ComPtr<ID3D12Device2> device,
		WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
		WRL::ComPtr<ID3D12CommandQueue> commandQueue,
		TextureInfo* pTextureInfo = nullptr);

	static ID3D12Resource* LoadTexture2DFromFile2(
		const std::string& fileName,
		ID3D12Device5* device,
		ID3D12GraphicsCommandList* commandList,
		ID3D12CommandQueue*		   commandQueue,
		TextureInfo* pTextureInfo = nullptr);

};