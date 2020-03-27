#pragma once
#include"Utility.h"
#include<vector>
#include<memory>
struct ObjSubmesh
{
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMFLOAT2 texcoord;
	};

	std::vector<ObjSubmesh::Vertex> vertices;
	std::vector<UINT>				indices;
};

using StandardVertex = ObjSubmesh::Vertex;

struct ObjMesh
{
	std::vector<ObjSubmesh> submeshs;
};
class ObjMeshLoader
{

public:
	static std::shared_ptr<ObjMesh> loadObjMeshFromFile(const std::string& filePath);
};