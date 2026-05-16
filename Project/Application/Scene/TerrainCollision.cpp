#include "TerrainCollision.h"
#include <algorithm>
#include <cmath>
#include "MyMath.h"

using namespace GameEngine;

void TerrainCollision::Build(Mesh& mesh,const float& cellSize)
{
	uint32_t vertCount = mesh.GetTotalVertices();
	uint32_t indexCount = mesh.GetTotalIndices();
	// 三角形の総数を取得
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

	// グリッド作成
	BuildGrid(cellSize);
}

void TerrainCollision::BuildGrid(const float& cellSize)
{
	if (vertices_.empty()) return;

	// メッシュ全体のXZ範囲を計算
	float minX = FLT_MAX;
	float maxX = -FLT_MAX;
	float minZ = FLT_MAX;
	float maxZ = -FLT_MAX;
	for (auto& v : vertices_) {
		minX = (std::min)(minX, v.x);
		maxX = (std::max)(maxX, v.x);
		minZ = (std::min)(minZ, v.z);
		maxZ = (std::max)(maxZ, v.z);
	}

	grid_.cellSize = cellSize;
	grid_.originX = minX;
	grid_.originZ = minZ;
	grid_.cellsX = static_cast<int>((maxX - minX) / cellSize) + 1;
	grid_.cellsZ = static_cast<int>((maxZ - minZ) / cellSize) + 1;
	grid_.cells.resize(grid_.cellsX * grid_.cellsZ);

	// 重なっているセルに各三角形を登録
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
		if (!result.hit) { continue; }

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

	// セルを取得
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

		float t;
		Vector3 normal;
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

	Vector3 edge1 = v1 - v0;
	Vector3 edge2 = v2 - v0;

	// レイと三角形面の傾き
	Vector3 h = Cross(rd, edge2);
	float a = Dot(edge1, h);
	// 平行確認
	if (std::abs(a) < EPSILON) { return false; }
	float f = 1.0f / a;

	/// 三角形の内側を確認
	Vector3 s = ro - v0;
	float u = f * Dot(s, h);
	if (u < 0.0f || u > 1.0f) { return false; }

	Vector3 q = Cross(s, edge1);
	float v = f * Dot(rd, q);
	if (v < 0.0f || u + v > 1.0f) { return false; }

	// レイの進んだ距離
	float t = f * Dot(edge2, q);
	// 後方・自己交差を除外
	if (t < EPSILON) { return false; }
	outT = t;

	// 法線
	Vector3 n = Cross(edge1, edge2);
	float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
	if (len > EPSILON) {
		outNormal = n / len;
	}

	return true;
}
