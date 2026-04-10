#include"Mesh.h"
#include <cassert>
#include <numbers>

using namespace GameEngine;

void Mesh::CreateTrianglePlaneMesh(ID3D12Device* device) {

	std::vector<VertexData> vertices(3);
	vertices[0].position = { -0.5f, -0.5f, 0.0f, 1.0f };
	vertices[0].texcoord = { 0.0f, 1.0f };
	vertices[0].normal = { 0.0f, 0.0f, -1.0f };
	// 上
	vertices[1].position = { 0.0f, 0.5f, 0.0f, 1.0f };
	vertices[1].texcoord = { 0.5f, 0.0f };
	vertices[1].normal = { 0.0f, 0.0f, -1.0f };
	// 右下
	vertices[2].position = { 0.5f, -0.5f, 0.0f, 1.0f };
	vertices[2].texcoord = { 1.0f, 1.0f };
	vertices[2].normal = { 0.0f, 0.0f, -1.0f };

	// 頂点データを作成
	vertexBuffer_.Create(device, vertices);
}

void Mesh::CreateGridPlaneMesh(ID3D12Device* device, const Vector2& size) {
	std::vector<VertexData> vertices(4);

	float left = -size.x / 2.0f;
	float right = size.x / 2.0f;
	float top = size.y / 2.0f;
	float bottom = -size.y / 2.0f;

	// 左上
	vertices[0].position = { left,0.0f,top,1.0f }; // 左下
	// 右上
	vertices[1].position = { right,0.0f,top,1.0f }; // 左上
	// 左下
	vertices[2].position = { left,0.0f,bottom,1.0f }; // 右下
	// 右下
	vertices[3].position = { right,0.0f,bottom,1.0f }; // 左上

	// 頂点データを作成
	vertexBuffer_.Create(device, vertices);


	// インデックスデータを生成
	std::vector<uint32_t> indices(6);
	// 三角形
	indices[0] = 0;  indices[1] = 1;  indices[2] = 2;
	// 三角形2
	indices[3] = 1;  indices[4] = 3;  indices[5] = 2;

	indexBuffer_.Create(device, indices);
}

void Mesh::CreatePlaneMesh(ID3D12Device* device, const Vector2& size) {

	std::vector<VertexData> vertices(4);

	float left = -size.x / 2.0f;
	float right = size.x / 2.0f;
	float top = size.y / 2.0f;
	float bottom = -size.y / 2.0f;

	// 左上
	vertices[0].position = { left,top,0.0f,1.0f };
	vertices[0].texcoord = { 0.0f,0.0f };
	vertices[0].normal = { 0.0f,0.0f,-1.0f };
	// 右上
	vertices[1].position = { right,top,0.0f,1.0f };
	vertices[1].texcoord = { 0.0f,0.0f };
	vertices[1].normal = { 0.0f,0.0f,-1.0f };
	// 左下
	vertices[2].position = { left,bottom,0.0f,1.0f };
	vertices[2].texcoord = { 0.0f,0.0f };
	vertices[2].normal = { 0.0f,0.0f,-1.0f };
	// 右下
	vertices[3].position = { right,bottom,0.0f,1.0f };
	vertices[3].texcoord = { 0.0f,0.0f };
	vertices[3].normal = { 0.0f,0.0f,-1.0f };

	// 頂点データを作成
	vertexBuffer_.Create(device, vertices);

	// インデックスデータを生成
	std::vector<uint32_t> indices(6);

	// 三角形
	indices[0] = 0;  indices[1] = 1; indices[2] = 2;
	// 三角形2
	indices[3] = 1;  indices[4] = 3;  indices[5] = 2;

	indexBuffer_.Create(device, indices);
}

void Mesh::CreateSphereMesh(ID3D12Device* device, uint32_t subdivision) {
	
	const float kLatEvery = std::numbers::pi_v<float> / static_cast<float>(subdivision);
	const float kLonEvery = 2.0f * std::numbers::pi_v<float> / static_cast<float>(subdivision);

	// 頂点生成
	std::vector<VertexData> vertices;
	for (uint32_t latIndex = 0; latIndex <= subdivision; ++latIndex) {
		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;
		float v = 1.0f - static_cast<float>(latIndex) / static_cast<float>(subdivision);
		for (uint32_t lonIndex = 0; lonIndex <= subdivision; ++lonIndex) {
			float lon = lonIndex * kLonEvery;
			float u = static_cast<float>(lonIndex) / static_cast<float>(subdivision);

			VertexData vertex;
			vertex.position = { std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon), 1.0f };
			vertex.texcoord = { u, v };
			vertex.normal = { vertex.position.x, vertex.position.y, vertex.position.z };
			vertices.push_back(vertex);
		}
	}

	vertexBuffer_.Create(device, vertices);

	// インデックス生成
	std::vector<uint32_t> indices;
	for (uint32_t latIndex = 0; latIndex < subdivision; ++latIndex) {
		for (uint32_t lonIndex = 0; lonIndex < subdivision; ++lonIndex) {
			uint32_t a = (latIndex * (subdivision + 1)) + lonIndex;
			uint32_t b = a + subdivision + 1;
			uint32_t c = a + 1;
			uint32_t d = b + 1;
			indices.insert(indices.end(), { a, b, d, a, d, c });
		}
	}

	indexBuffer_.Create(device, indices);
}

void Mesh::CreateModelMesh(ID3D12Device* device,ModelData modelData, const uint32_t& index) {

	// 要素が無ければエラー
	assert(modelData.meshes.size() > index);

	const auto& meshData = modelData.meshes[index];
	materialName_ = meshData.materialName;

	vertexBuffer_.Create(device, meshData.vertices);
	indexBuffer_.Create(device, meshData.indices);
}