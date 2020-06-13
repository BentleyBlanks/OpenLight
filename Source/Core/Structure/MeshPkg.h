#pragma once
#include "ObjMeshLoader.h"
#include "CBStruct.h"
#include<memory>
struct MeshPkg
{
	MeshPkg() {}
	~MeshPkg() 
	{
		ReleaseCom(vb);
		ReleaseCom(ib);
	}
	std::string							name;
	std::shared_ptr<ObjMesh>			mesh;
	ID3D12Resource*						vb = nullptr;
	D3D12_VERTEX_BUFFER_VIEW			vbView;
	ID3D12Resource*						ib = nullptr;
	D3D12_INDEX_BUFFER_VIEW				ibView;
	XMFLOAT4X4							world;
	CBMaterial							material;
	MaterialTexture						materialTexture;

	bool								visible = true;
	bool								transparent = false;
	bool								useTexture = false;
	bool								materialChanged = false;
};
