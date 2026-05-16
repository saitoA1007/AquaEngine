#include "TerrainCollision.h"
#include <algorithm>
#include <cmath>

using namespace GameEngine;

void TerrainCollision::Build(Mesh& mesh)
{
	uint32_t vertCount = mesh.GetTotalVertices();
	uint32_t indexCount = mesh.GetTotalIndices();
	triangleCount_ = indexCount / 3;

	// 頂点データを取得
	const std::vector<VertexData>& vdata = mesh.veretxData_;
	vertices_.resize(vertCount);
	for (uint32_t i = 0; i < vertCount; ++i) {
		vertices_[i] = {
			vdata[i].position.x,
			vdata[i].position.y,
			vdata[i].position.z
		};
	}

	// インデックスデータを取得
	const std::vector<uint32_t>& idata = mesh.indexData_;
	indices_.resize(indexCount);
	for (uint32_t i = 0; i < indexCount; ++i) {
		indices_[i] = idata[i];
	}

	// 三角形のAABBを事前に計算する
	triAABBs_.resize(triangleCount_);
	for (uint32_t i = 0; i < triangleCount_; ++i) {
		const Vector3& v0 = vertices_[indices_[i * 3 + 0]];
		const Vector3& v1 = vertices_[indices_[i * 3 + 1]];
		const Vector3& v2 = vertices_[indices_[i * 3 + 2]];
		triAABBs_[i] = {
			(std::min)({v0.x, v1.x, v2.x}),
			(std::max)({v0.x, v1.x, v2.x}),
			(std::min)({v0.z, v1.z, v2.z}),
			(std::max)({v0.z, v1.z, v2.z})
		};
	}
}

void TerrainCollision::BuildGrid(float cellSize)
{
	if (vertices_.empty()) return;

	// メッシュ全体のXZ範囲を計算
	float minX = FLT_MAX, maxX = -FLT_MAX;
	float minZ = FLT_MAX, maxZ = -FLT_MAX;
	for (auto& v : vertices_) {
		minX = (std::min)(minX, v.x); maxX = (std::max)(maxX, v.x);
		minZ = (std::min)(minZ, v.z); maxZ = (std::max)(maxZ, v.z);
	}

	grid_.cellSize = cellSize;
	grid_.originX = minX;
	grid_.originZ = minZ;
	grid_.cellsX = static_cast<int>((maxX - minX) / cellSize) + 1;
	grid_.cellsZ = static_cast<int>((maxZ - minZ) / cellSize) + 1;
	grid_.cells.resize(grid_.cellsX * grid_.cellsZ);

	// 各三角形を重なるセルに登録
	for (uint32_t i = 0; i < triangleCount_; ++i) {
		const auto& aabb = triAABBs_[i];
		int ixMin, izMin, ixMax, izMax;
		// AABBが跨ぐセル範囲を求める
		ixMin = (std::max)(0, (int)((aabb.minX - grid_.originX) / cellSize));
		ixMax = (std::min)(grid_.cellsX - 1, (int)((aabb.maxX - grid_.originX) / cellSize));
		izMin = (std::max)(0, (int)((aabb.minZ - grid_.originZ) / cellSize));
		izMax = (std::min)(grid_.cellsZ - 1, (int)((aabb.maxZ - grid_.originZ) / cellSize));

		for (int iz = izMin; iz <= izMax; ++iz) {
			for (int ix = ixMin; ix <= ixMax; ++ix) {
				grid_.cells[grid_.CellIndex(ix, iz)].push_back(i);
			}
		}
	}
}

void TerrainCollision::ResolveCollision(std::vector<PlayerData>& players) const
{
	for (auto& player : players)
	{
		GroundResult result = GetGroundHeight(player.position);
		if (!result.hit) continue;

		float groundTop = result.groundY + player.height;
		if (player.position.y < groundTop)
		{
			player.position.y = groundTop;
		}
	}
}

GroundResult TerrainCollision::GetGroundHeight(const Vector3& position) const
{
	GroundResult result{};
	result.hit = false;
	result.groundY = 0.0f;
	result.normal = { 0.0f, 1.0f, 0.0f };


	if (vertices_.empty() || indices_.empty()) { return result; }

	int ix, iz;
	if (!grid_.WorldToCell(position.x, position.z, ix, iz)) { return result; }

	const auto& candidates = grid_.cells[grid_.CellIndex(ix, iz)];

	// 真上から下方向にレイを飛ばす
	Vector3 rayOrigin = { position.x, position.y + 100.0f, position.z };
	Vector3 rayDir = { 0.0f, -1.0f, 0.0f };
	float   closestT = FLT_MAX;
	Vector3 closestNormal = { 0.0f, 1.0f, 0.0f };

	// セルの中で回す
	for (uint32_t triIdx : candidates) {        
		const Vector3& v0 = vertices_[indices_[triIdx * 3 + 0]];
		const Vector3& v1 = vertices_[indices_[triIdx * 3 + 1]];
		const Vector3& v2 = vertices_[indices_[triIdx * 3 + 2]];

		float t; Vector3 normal;
		if (RayTriangleIntersect(rayOrigin, rayDir, v0, v1, v2, t, normal)) {
			if (t < closestT) {
				closestT = t;
				closestNormal = normal;
				result.hit = true;
			}
		}
	}

	if (result.hit) {
		result.groundY = rayOrigin.y + rayDir.y * closestT;
		result.normal = closestNormal;
	}
	return result;
}

bool TerrainCollision::RayTriangleIntersect(
	const Vector3& ro, const Vector3& rd,
	const Vector3& v0, const Vector3& v1, const Vector3& v2,
	float& outT, Vector3& outNormal) const
{
	constexpr float EPSILON = 1e-6f;

	Vector3 edge1 = { v1.x - v0.x, v1.y - v0.y, v1.z - v0.z };
	Vector3 edge2 = { v2.x - v0.x, v2.y - v0.y, v2.z - v0.z };

	// h = cross(rd, edge2)
	Vector3 h = {
		rd.y * edge2.z - rd.z * edge2.y,
		rd.z * edge2.x - rd.x * edge2.z,
		rd.x * edge2.y - rd.y * edge2.x
	};

	float a = edge1.x * h.x + edge1.y * h.y + edge1.z * h.z;
	if (std::abs(a) < EPSILON) return false; // 平行

	float f = 1.0f / a;

	Vector3 s = { ro.x - v0.x, ro.y - v0.y, ro.z - v0.z };
	float u = f * (s.x * h.x + s.y * h.y + s.z * h.z);
	if (u < 0.0f || u > 1.0f) return false;

	// q = cross(s, edge1)
	Vector3 q = {
		s.y * edge1.z - s.z * edge1.y,
		s.z * edge1.x - s.x * edge1.z,
		s.x * edge1.y - s.y * edge1.x
	};

	float v = f * (rd.x * q.x + rd.y * q.y + rd.z * q.z);
	if (v < 0.0f || u + v > 1.0f) return false;

	float t = f * (edge2.x * q.x + edge2.y * q.y + edge2.z * q.z);
	if (t < EPSILON) return false; // 後方・自己交差を除外

	outT = t;

	// 法線 = normalize(cross(edge1, edge2))
	Vector3 n = {
		edge1.y * edge2.z - edge1.z * edge2.y,
		edge1.z * edge2.x - edge1.x * edge2.z,
		edge1.x * edge2.y - edge1.y * edge2.x
	};
	float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
	if (len > EPSILON) {
		outNormal = { n.x / len, n.y / len, n.z / len };
	}

	return true;
}
