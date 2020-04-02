#pragma once
#include"ObjMeshLoader.h"
#include"UploadBuffer.h"
#include"Material.h"
#include"Heap.h"
#include"AppConfig.h"
namespace OpenLight
{
	template<typename OBJCB,typename MATERIAL>
	class MeshBatch
	{
	public:
		MeshBatch(const MATERIAL& mat, const ObjSubmesh& mesh,
			const OBJCB& cb,
			ID3D12Device* device,
			UploadBufferHeap* heap)
		{
			mMaterial	= mat;
			mMesh		= mesh;
			mCB			= cb;

			mUploadMaterial =	new UploadBuffer<MATERIAL>(device, 1, heap);
			mUploadOBJCB	=	new UploadBuffer<OBJCB>(device, 1, heap);
		}
		MeshBatch(const MeshBatch&)				= delete;
		MeshBatch& operator= (const MeshBatch&) = delete;

		void updateCB(UINT frameIndex,const OBJCB& cb)
		{
			mCB = cb;
			mUploadOBJCB->CopyData(frameIndex, mCB);
		}
		void updateMaterial(UINT frameIndex, const MATERIAL& mtl)
		{
			mMaterial = mtl;
			mUploadMaterial->CopyData(frameIndex, mMaterial);
		}

	protected:
		MATERIAL	mMaterial;
		ObjSubmesh	mMesh;
		OBJCB		mCB;
		std::unique_ptr<UploadBuffer<MATERIAL>> mUploadMaterial;
		std::unique_ptr<UploadBuffer<OBJCB>>	mUploadOBJCB;
	};
}