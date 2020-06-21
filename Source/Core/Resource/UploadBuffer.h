#pragma once
#include "Utility.h"
#include<d3d12.h>
#include"d3dx12.h"
namespace OpenLight
{

	template<typename T>
	class UploadBuffer
	{
	public:
		UploadBuffer(ID3D12Device* device, UINT elementCount, UploadBufferHeap* heap) :
			mIsConstantBuffer(isConstantBuffer)
		{

			if (!heap->allocateUploadBuffer<T>(device, elementCount, mUploadBuffer))
			{
				assert(false);
			}

			ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

			// We do not need to unmap until we are done with the resource.  However, we must not write to
			// the resource while it is in use by the GPU (so we must use synchronization techniques).
		}

		UploadBuffer(const UploadBuffer& rhs) = delete;
		UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
		~UploadBuffer()
		{
			if (mUploadBuffer != nullptr)
				mUploadBuffer->Unmap(0, nullptr);

			mMappedData = nullptr;
		}

		ID3D12Resource* Resource()const
		{
			return mUploadBuffer.Get();
		}

		void CopyData(int elementIndex, const T& data)
		{
			memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T));
		}

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
		BYTE* mMappedData = nullptr;

		UINT mElementByteSize = 0;
		bool mIsConstantBuffer = false;
	};
}