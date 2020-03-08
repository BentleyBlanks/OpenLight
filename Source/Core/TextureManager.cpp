#include "TextureManager.h"
#include "Utility.h"

using namespace DirectX;
void loadTextureFromFile(const std::string& fileName,
	std::vector<XMFLOAT4>* rawData,
	int* pWidth,
	int* pHeight)
{
	#if 1
auto fullPath = fileName;
	FreeImage_Initialise(true);


	FIBITMAP* bmpConverted = nullptr;
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	//��ȡ�ļ�����
	fif = FreeImage_GetFileType(fullPath.c_str());
	if (fif == FIF_UNKNOWN)
		fif = FreeImage_GetFIFFromFilename(fullPath.c_str());


	//�������� ���
	FIBITMAP* dib = FreeImage_Load(fif, fullPath.c_str());


	//������Ҫ��������
	//	FreeImage_FlipVertical(dib);

	//�����������ݽṹ

	//��ȡ�����������Ϣ
	auto width = FreeImage_GetWidth(dib);
	auto height = FreeImage_GetHeight(dib);
	auto bpp = FreeImage_GetBPP(dib);
	auto colorType = FreeImage_GetColorType(dib);



	std::vector<XMFLOAT4> data(width *height);

	if (fif != FIF_EXR)
	{
		RGBQUAD rgb;
		if (bpp != 24)
			dib = FreeImage_ConvertTo24Bits(dib);
		for (UINT y = 0; y < height; ++y)
		{
			for (UINT x = 0; x < width; ++x)
			{
				FreeImage_GetPixelColor(dib, x, y, &rgb);
				//rgb�У�ÿ������ΪBYTE����(0~255)
				//����ת��Ϊ[0,1]��f32�ͣ���������Spectrum��
				float r, g, b;
				r = float(rgb.rgbRed) / 255.f;
				g = float(rgb.rgbGreen) / 255.f;
				b = float(rgb.rgbBlue) / 255.f;

				r = std::powf(r, 2.2f);
				g = std::powf(g, 2.2f);
				b = std::powf(b, 2.2f);



				data[y*width + x] = XMFLOAT4(r, g, b, 1.f);
			}
		}
	}
	else
	{
		float maxV = 0.f;
		FIBITMAP* rgbfBitmap = FreeImage_ConvertToRGBF(dib);

		FIRGBF* image = (FIRGBF*)FreeImage_GetBits(rgbfBitmap);
		for (UINT y = 0; y < height; ++y)
		{
			for (UINT x = 0; x < width; ++x)
			{

				FIRGBF rgb = image[y*width + x];
				if (rgb.red > maxV) maxV = rgb.red;
				if (rgb.green > maxV) maxV = rgb.green;
				if (rgb.blue > maxV) maxV = rgb.blue;
				data[y*width + x] = XMFLOAT4(rgb.red, rgb.green, rgb.blue, 1.f);
			}
		}
	}



	if (rawData)
	{
		rawData->resize(data.size());
		std::memcpy(&((*rawData)[0]), &data[0],
			sizeof(XMFLOAT4)* data.size());
	}

	if (pWidth)
		*pWidth = width;
	if (pHeight)
		*pHeight = height;
#endif
}

WRL::ComPtr<ID3D12Resource1> TextureMgr::LoadTexture2DFromFile(const std::string & fileName,
	WRL::ComPtr<ID3D12Device2> device,
	WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
	WRL::ComPtr<ID3D12CommandQueue> commandQueue,
	TextureInfo * pTextureInfo)
{
	std::vector<XMFLOAT4> rawData;
	int width;
	int height;

	loadTextureFromFile(fileName, &rawData, &width, &height);

	WRL::ComPtr<ID3D12Resource1> texture;
	WRL::ComPtr<ID3D12Resource1> textureUpload;

	// ��Ĭ�϶��ϴ�����Դ
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, 1, 1),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture)));

	// ��ȡʵ�ʵĴ�С
	UINT64 uploadBufferSize;
	device->GetCopyableFootprints(
		&texture->GetDesc(),
		0, 1,
		0, nullptr, nullptr, nullptr, &uploadBufferSize);
#if 1
	// �����ϴ����ϵ���Դ
	auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&upDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUpload)));

	// ���������������ϴ���
	// ��ȡ�ϴ���Ŀ��ĸ�ʽ����Ϣ
	auto uploadDesc = textureUpload->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT texLayout = {};
	UINT64 requiredSize = 0;
	UINT64 textureRowSize = 0;
	UINT textureRowNum = 0;

	device->GetCopyableFootprints(
		&texture->GetDesc(),
		0,
		1,
		0,
		&texLayout,
		&textureRowNum,
		&textureRowSize,
		&requiredSize);

	BYTE* pData = nullptr;
	ThrowIfFailed(textureUpload->Map(0, nullptr, reinterpret_cast<void**>(&pData)));

	// ��ȡ�ϴ��ѵĿ�ʼָ��λ��
	BYTE* pDest = reinterpret_cast<BYTE*>(pData) + texLayout.Offset;
	BYTE* pSrc = reinterpret_cast<BYTE*>(&rawData[0]);
	SIZE_T srcRowPitch = sizeof(XMFLOAT4) * width;
	for(UINT y = 0;y<textureRowNum;++y)
	{
		memcpy(pDest + static_cast<SIZE_T>(texLayout.Footprint.RowPitch) * y,
			pSrc + srcRowPitch * y,
			srcRowPitch);
	}
	textureUpload->Unmap(0,nullptr);


	// ��������д��ݸ�������
	CD3DX12_TEXTURE_COPY_LOCATION dest(texture.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION src(textureUpload.Get(), texLayout);
	commandList->CopyTextureRegion(
		&dest, 0, 0, 0,
		&src, nullptr);

	// ������Դ����
	D3D12_RESOURCE_BARRIER resBar = {};
	resBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resBar.Transition.pResource = texture.Get();
	resBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	resBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	resBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1, &resBar);

	ThrowIfFailed(commandList->Close());
	// ִ�������б�
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, ppCommandLists);

	// ���� Fence ����ͬ��
	WRL::ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	// ͬ������
	ThrowIfFailed(commandQueue->Signal(fence.Get(), 1));
	auto fenceEvent = CreateEvent(nullptr, false, false, nullptr);

	if (fence->GetCompletedValue() < 1)
	{
		fence->SetEventOnCompletion(1, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
#endif

	// ���ش����õ� Ĭ�϶�����
	return texture;
}


