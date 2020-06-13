#include "ObjMeshLoader.h"
#include "tiny_obj_loader.h"
#include <iostream>
void calTangentAndFill(std::shared_ptr<ObjMesh> mesh,
	std::vector<StandardVertex>& vertices,
	const std::vector<UINT>& indices)
{

	for (int i = 0; i < mesh->submeshs[0].indices.size() / 3; ++i)
	{

		auto& v0 = mesh->submeshs[0].vertices[indices[i * 3 + 0]];
		auto& v1 = mesh->submeshs[0].vertices[indices[i * 3 + 1]];
		auto& v2 = mesh->submeshs[0].vertices[indices[i * 3 + 2]];

		auto e0 = v1.position - v0.position;
		auto e1 = v2.position - v0.position;



		float du0 = v1.texcoord.x - v0.texcoord.x;
		float dv0 = v1.texcoord.y - v0.texcoord.y;

		float du1 = v2.texcoord.x - v0.texcoord.x;
		float dv1 = v2.texcoord.y - v0.texcoord.y;

		/*
			当 顶点没有纹理坐标时
			v0 : (0 0)
			v1 : (1 0)
			v2 : (0 1)
		*/
		if (du0 == 0.f && dv0 == 0.f && du1 == 0.f && dv1 == 0.f)
		{
			du0 = 1;
			dv0 = 0;
			du1 = 0;
			dv1 = 1;
		}

		float det = 1.f / (du0*dv1 - dv0 * du1);
		XMFLOAT3 tangent = XMFLOAT3(
			det * (dv1*e0.x - dv0 * e1.x),
			det * (dv1*e0.y - dv0 * e1.y),
			det * (dv1*e0.z - dv0 * e1.z)
		);

		StandardVertex vertex0;
		StandardVertex vertex1;
		StandardVertex vertex2;

		vertex0.position = v0.position;
		vertex0.normal = v0.normal;
		vertex0.tangent = tangent;
		vertex0.texcoord = v0.texcoord;

		vertex1.position = v1.position;
		vertex1.normal = v1.normal;
		vertex1.tangent = tangent;
		vertex1.texcoord = v1.texcoord;

		vertex2.position = v2.position;
		vertex2.normal = v2.normal;
		vertex2.tangent = tangent;
		vertex2.texcoord = v2.texcoord;

		vertices.push_back(vertex0);
		vertices.push_back(vertex1);
		vertices.push_back(vertex2);
	}
}


void calTangentAndFill(ObjSubmesh& submesh)
{
	auto& indices = submesh.indices;
	std::vector<StandardVertex> vertices;
	for (int i = 0; i < submesh.indices.size() / 3; ++i)
	{

		auto& v0 = submesh.vertices[indices[i * 3 + 0]];
		auto& v1 = submesh.vertices[indices[i * 3 + 1]];
		auto& v2 = submesh.vertices[indices[i * 3 + 2]];

		auto e0 = v1.position - v0.position;
		auto e1 = v2.position - v0.position;



		float du0 = v1.texcoord.x - v0.texcoord.x;
		float dv0 = v1.texcoord.y - v0.texcoord.y;

		float du1 = v2.texcoord.x - v0.texcoord.x;
		float dv1 = v2.texcoord.y - v0.texcoord.y;

		/*
			当 顶点没有纹理坐标时
			v0 : (0 0)
			v1 : (1 0)
			v2 : (0 1)
		*/
		if (du0 == 0.f && dv0 == 0.f && du1 == 0.f && dv1 == 0.f)
		{
			du0 = 1;
			dv0 = 0;
			du1 = 0;
			dv1 = 1;
		}

		float det = 1.f / (du0*dv1 - dv0 * du1);
		XMFLOAT3 tangent = XMFLOAT3(
			det * (dv1*e0.x - dv0 * e1.x),
			det * (dv1*e0.y - dv0 * e1.y),
			det * (dv1*e0.z - dv0 * e1.z)
		);

		StandardVertex vertex0;
		StandardVertex vertex1;
		StandardVertex vertex2;

		vertex0.position = v0.position;
		vertex0.normal = v0.normal;
		vertex0.tangent = tangent;
		vertex0.texcoord = v0.texcoord;

		vertex1.position = v1.position;
		vertex1.normal = v1.normal;
		vertex1.tangent = tangent;
		vertex1.texcoord = v1.texcoord;

		vertex2.position = v2.position;
		vertex2.normal = v2.normal;
		vertex2.tangent = tangent;
		vertex2.texcoord = v2.texcoord;

		vertices.push_back(vertex0);
		vertices.push_back(vertex1);
		vertices.push_back(vertex2);
	}

	// 拷贝回去
	submesh.vertices = std::move(vertices);
}

std::shared_ptr<ObjMesh> ObjMeshLoader::loadObjMeshFromFile(const std::string & filePath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	std::string warn;


	if (!tinyobj::LoadObj(&attrib, &shapes,
		&materials, &err, &warn, filePath.c_str(), nullptr, true))
	{
		std::cout << err << std::endl;
		std::cout << warn << std::endl;
	}

	std::shared_ptr<ObjMesh> mesh(new ObjMesh());

	for (size_t s = 0; s < shapes.size(); ++s)
	{
		ObjSubmesh submesh;
		size_t indexOffset = 0;

		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f)
		{
			auto fv = shapes[s].mesh.num_face_vertices[f];

			XMFLOAT3 p = XMFLOAT3(0.f, 0.f, 0.f);
			XMFLOAT3 n = XMFLOAT3(0.f, 0.f, 0.f);
			XMFLOAT2 t = XMFLOAT2(0.f, 0.f);

			for (size_t v = 0; v < fv; ++v)
			{
				auto idx = shapes[s].mesh.indices[indexOffset + v];
				auto vx = attrib.vertices[3 * idx.vertex_index + 0];
				auto vy = attrib.vertices[3 * idx.vertex_index + 1];
				auto vz = attrib.vertices[3 * idx.vertex_index + 2];

				p = XMFLOAT3(vx, vy, vz);

				if (!attrib.normals.empty())
				{
					auto nx = attrib.normals[3 * idx.normal_index + 0];
					auto ny = attrib.normals[3 * idx.normal_index + 1];
					auto nz = attrib.normals[3 * idx.normal_index + 2];
					n = XMFLOAT3(nx, ny, nz);
				}
				if (!attrib.texcoords.empty())
				{
					auto tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					auto ty = attrib.texcoords[2 * idx.texcoord_index + 1];
					t = XMFLOAT2(tx, ty);
				}
				ObjSubmesh::Vertex vertex;
				vertex.position = p;
				vertex.normal = n;
				vertex.texcoord = t;

				submesh.indices.push_back(submesh.vertices.size());
				submesh.vertices.push_back(vertex);

			}
			indexOffset += fv;
		}
		calTangentAndFill(submesh);
		mesh->submeshs.push_back(std::move(submesh));
	}

	return mesh;
}
