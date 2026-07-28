#define NOMINMAX
#include <algorithm>
#include "FractureInstance.h"
#include "MyMath.h"
#include "RandomGenerator.h"
using namespace GameEngine;

FractureInstance::~FractureInstance() {

}
void FractureInstance::Initialize(const std::vector<uint32_t>& chunkIds, const PackedGeometryBuffer& geometryBuffer) {

	// チャンクの数
	numInstance_ = static_cast<uint32_t>(chunkIds.size());

	// リソース作成
	buffer_.Release();
	buffer_.Create(numInstance_);
	instancingData_ = buffer_.GetData();

	// CPU側のトランスフォーム配列も同じサイズに確保
	transformData_.resize(numInstance_);

	argumentBuffer_.Release();
	argumentBuffer_.Create(numInstance_);
	argumentData_ = argumentBuffer_.GetData();

	for (uint32_t index = 0; index < numInstance_; ++index) {
		// PackedGeometryBufferからこのチャンクの描画範囲を取得
		uint32_t chunkId = chunkIds[index];
		const GeometryRange& range = geometryBuffer.GetRange(chunkId);

		// トランスフォームを初期化
		transformData_[index].transform.scale = { 1.0f, 1.0f, 1.0f };
		transformData_[index].transform.rotate = { 0.0f, 0.0f, 0.0f };
		transformData_[index].transform.translate = { 0.0f, 0.0f, 0.0f };
		instancingData_[index].world = Matrix4x4::MakeIdentity();
		instancingData_[index].worldInverseTranspose = Matrix4x4::MakeIdentity();

		// PackedGeometryBufferから得た描画範囲を設定
		instancingData_[index].vertexOffset = range.vertexOffset;
		instancingData_[index].indexOffset = range.indexOffset;
		instancingData_[index].indexCount = range.indexCount;
		instancingData_[index].chunkId = chunkId;

		// ExecuteIndirect用のDrawコマンド引数を設定
		argumentData_[index].instanceIndex = index;
		argumentData_[index].drawArguments.IndexCountPerInstance = range.indexCount; // 破片のインデックス数
		argumentData_[index].drawArguments.InstanceCount = 1;                        // 1回につき1個描画
		argumentData_[index].drawArguments.StartIndexLocation = range.indexOffset;   // インデックスの開始位置
		argumentData_[index].drawArguments.BaseVertexLocation = static_cast<INT>(range.vertexOffset); // 頂点の開始位置
		argumentData_[index].drawArguments.StartInstanceLocation = 0;
	}
}

void FractureInstance::Update() {
	// 更新
	for (uint32_t i = 0; i < transformData_.size(); ++i) {
		instancingData_[i].world = Math::MakeAffineMatrix(transformData_[i].transform.scale, transformData_[i].transform.rotate, transformData_[i].transform.translate);
		instancingData_[i].worldInverseTranspose = Math::InverseTranspose(instancingData_[i].world);
	}
}

void FractureInstance::InitializeFromRanges(const std::vector<GeometryRange>& ranges) {
	AllocateBuffers(static_cast<uint32_t>(ranges.size()));
	for (uint32_t index = 0; index < numInstance_; ++index) {
		// ランタイム破片には元のchunkIdがないのでindexを使用
		WriteInstance(index, ranges[index], index);
	}
}

void FractureInstance::ApplyRuntimeCut(const Fragment& source, const Vector3& impactPos, float craterRadius, int numSites, int planeCount) {
	std::vector<VertexData> carvedVerts = source.vertices;
	std::vector<uint32_t> carvedIndices = source.indices;

	// 衝撃点周りを、衝突形状に合わせて凹ませる
	CarveImpactCrater(carvedVerts, carvedIndices, impactPos, craterRadius, planeCount);

	// クレーターで全部消えてしまった場合は元のメッシュにフォールバック
	if (carvedIndices.empty()) {
		carvedVerts = source.vertices;
		carvedIndices = source.indices;
	}

	int maxReasonableSites = std::max(2, static_cast<int>((carvedIndices.size() / 3) / kMinTriangleCount));
	int clampedSites = std::min(numSites, maxReasonableSites);

	std::vector<Fragment> fragments;
	VoronoiFracture(carvedVerts, carvedIndices, impactPos, clampedSites, fragments);

	runtimeBuffer_ = std::make_unique<RuntimeFractureBuffer>();
	std::vector<GeometryRange> ranges = runtimeBuffer_->Upload(fragments);
	InitializeFromRanges(ranges);
}

void FractureInstance::AllocateBuffers(uint32_t count) {
	numInstance_ = count;

	buffer_.Create(numInstance_);
	instancingData_ = buffer_.GetData();

	transformData_.resize(numInstance_);

	argumentBuffer_.Create(numInstance_);
	argumentData_ = argumentBuffer_.GetData();
}

void FractureInstance::WriteInstance(uint32_t index, const GeometryRange& range, uint32_t chunkId) {
	transformData_[index].transform.scale = { 1.0f, 1.0f, 1.0f };
	transformData_[index].transform.rotate = { 0.0f, 0.0f, 0.0f };
	transformData_[index].transform.translate = { 0.0f, 0.0f, 0.0f };

	instancingData_[index].world = Matrix4x4::MakeIdentity();
	instancingData_[index].worldInverseTranspose = Matrix4x4::MakeIdentity();
	instancingData_[index].vertexOffset = range.vertexOffset;
	instancingData_[index].indexOffset = range.indexOffset;
	instancingData_[index].indexCount = range.indexCount;
	instancingData_[index].chunkId = chunkId;

	argumentData_[index].instanceIndex = index;
	argumentData_[index].drawArguments.IndexCountPerInstance = range.indexCount;
	argumentData_[index].drawArguments.InstanceCount = 1;
	argumentData_[index].drawArguments.StartIndexLocation = range.indexOffset;
	argumentData_[index].drawArguments.BaseVertexLocation = static_cast<INT>(range.vertexOffset);
	argumentData_[index].drawArguments.StartInstanceLocation = 0;
}

ClipResult FractureInstance::ClipMeshByPlane(const std::vector<VertexData>& verts,
	const std::vector<uint32_t>& indices,
	const Vector3& planeNormal, float planeDist) {

	ClipResult result;
	// キャッピング用の切断エッジ
	std::vector<std::pair<VertexData, VertexData>> cutEdges;

	auto SignedDist = [&](const Vector3& p) {
		return Math::Dot(planeNormal, p) - planeDist;
		};

	for (size_t i = 0; i < indices.size(); i += 3) {
		VertexData v[3] = { verts[indices[i]], verts[indices[i + 1]], verts[indices[i + 2]] };
		float d[3] = { SignedDist(Vector3(v[0].position.x, v[0].position.y, v[0].position.z)),
			SignedDist(Vector3(v[1].position.x, v[1].position.y, v[1].position.z)),
			SignedDist(Vector3(v[2].position.x, v[2].position.y, v[2].position.z)) };

		// 3頂点が全部同じ側であればそのまま片方に追加
		if (d[0] >= 0 && d[1] >= 0 && d[2] >= 0) { 
			AddTriangle(result.frontVerts, result.frontIndices, v);
			continue; 
		}

		if (d[0] < 0 && d[1] < 0 && d[2] < 0) { 
			AddTriangle(result.backVerts, result.backIndices, v); 
			continue;
		}

		// 平面をまたぐ三角形からエッジとの交点を計算し、front,backに三角形分割して振り分け
		SplitStraddlingTriangle(v, d, planeNormal, planeDist, result, cutEdges);
	}

	// 切断エッジ群をつないで多角形化から三角形ファンでキャップし、front,back両方に追加
	CapCutFace(cutEdges, planeNormal, result);

	return result;
}

void FractureInstance::SplitStraddlingTriangle(const VertexData v[3], const float d[3],
	const Vector3& planeNormal, float planeDist,
	ClipResult& result, std::vector<std::pair<VertexData, VertexData>>& cutEdges) {

	(void)planeDist;

	auto Lerp = [](const VertexData& a, const VertexData& b, float t) {
		VertexData r;
		r.position = a.position + (b.position - a.position) * t;
		r.normal = Math::Normalize(a.normal + (b.normal - a.normal) * t);
		r.texcoord = a.texcoord + (b.texcoord - a.texcoord) * t;
		return r;
		};

	// 3頂点のうち他の2つと逆側にいる1頂点を探す
	int loneIndex = -1;
	bool loneIsFront = false;
	for (int i = 0; i < 3; ++i) {
		int o1 = (i + 1) % 3, o2 = (i + 2) % 3;
		bool iFront = d[i] >= 0.0f;
		bool o1Front = d[o1] >= 0.0f;
		bool o2Front = d[o2] >= 0.0f;
		if (iFront != o1Front && iFront != o2Front) {
			loneIndex = i;
			loneIsFront = iFront;
			break;
		}
	}
	if (loneIndex < 0) { return; }

	int a = loneIndex, b = (loneIndex + 1) % 3, c = (loneIndex + 2) % 3;

	float tAB = d[a] / (d[a] - d[b]);
	float tAC = d[a] / (d[a] - d[c]);
	VertexData pAB = Lerp(v[a], v[b], tAB);
	VertexData pAC = Lerp(v[a], v[c], tAC);

	VertexData loneTri[3] = { v[a], pAB, pAC };
	VertexData quad1[3] = { pAB, v[b], v[c] };
	VertexData quad2[3] = { pAB, v[c], pAC };

	if (loneIsFront) {
		AddTriangle(result.frontVerts, result.frontIndices, loneTri);
		AddTriangle(result.backVerts, result.backIndices, quad1);
		AddTriangle(result.backVerts, result.backIndices, quad2);
		cutEdges.push_back({ pAB, pAC });
	} else {
		AddTriangle(result.backVerts, result.backIndices, loneTri);
		AddTriangle(result.frontVerts, result.frontIndices, quad1);
		AddTriangle(result.frontVerts, result.frontIndices, quad2);
		cutEdges.push_back({ pAC, pAB });
	}
}

void FractureInstance::CapCutFace(const std::vector<std::pair<VertexData, VertexData>>& cutEdges,
	const Vector3& planeNormal, ClipResult& result) {

	if (cutEdges.empty()) { return; }

	std::vector<std::pair<VertexData, VertexData>> remaining(cutEdges.begin(), cutEdges.end());
	std::vector<VertexData> loop;

	loop.push_back(remaining.front().first);
	VertexData current = remaining.front().second;
	remaining.erase(remaining.begin());

	constexpr float kEps = 1e-4f;
	while (!remaining.empty()) {
		loop.push_back(current);
		bool found = false;
		for (size_t i = 0; i < remaining.size(); ++i) {
			if (Math::Length(remaining[i].first.position - current.position) < kEps) {
				current = remaining[i].second;
				remaining.erase(remaining.begin() + i);
				found = true;
				break;
			}
			if (Math::Length(remaining[i].second.position - current.position) < kEps) {
				current = remaining[i].first;
				remaining.erase(remaining.begin() + i);
				found = true;
				break;
			}
		}
		if (!found) { break; }
	}
	if (loop.size() < 3) { return; }

	// front面
	for (size_t i = 1; i + 1 < loop.size(); ++i) {
		VertexData tri[3] = { loop[0], loop[i], loop[i + 1] };
		for (auto& vtx : tri) { 
			vtx.normal = planeNormal; 
		}
		AddTriangle(result.frontVerts, result.frontIndices, tri);
	}
	// back面
	for (size_t i = 1; i + 1 < loop.size(); ++i) {
		VertexData tri[3] = { loop[0], loop[i + 1], loop[i] };
		for (auto& vtx : tri) { 
			vtx.normal = planeNormal * -1.0f; 
		}
		AddTriangle(result.backVerts, result.backIndices, tri);
	}
}

std::vector<Vector3> FractureInstance::GenerateVoronoiSites(const AABB& bounds, const Vector3& impactPos, int numSites) const {
	std::vector<Vector3> sites;
	sites.reserve(numSites);

	Vector3 extent = bounds.max - bounds.min;
	float maxRadius = Math::Length(extent) * 0.5f;

	for (int i = 0; i < numSites; ++i) {
		// tが0に近いほど衝撃点の近くに配置される
		float t = RandomGenerator::Get(0.0f, 1.0f);
		float radius = maxRadius * (t * t);

		Vector3 dir = RandomGenerator::GetVector3(-1.0f, 1.0f);
		dir.Normalize();

		Vector3 pos = impactPos + dir * radius;

		// バウンディングボックス外だとクリップが噛み合わずセルが消えやすいのでクランプ
		pos.x = std::clamp(pos.x, bounds.min.x, bounds.max.x);
		pos.y = std::clamp(pos.y, bounds.min.y, bounds.max.y);
		pos.z = std::clamp(pos.z, bounds.min.z, bounds.max.z);

		sites.push_back(pos);
	}
	return sites;
}

void FractureInstance::VoronoiFracture(const std::vector<VertexData>& verts, const std::vector<uint32_t>& indices,
	const Vector3& impactPos, int numSites, std::vector<Fragment>& outFragments) {

	if (numSites <= 1) {
		outFragments.push_back({ verts, indices });
		return;
	}

	AABB bounds = ComputeBounds(verts);
	std::vector<Vector3> sites = GenerateVoronoiSites(bounds, impactPos, numSites);

	for (size_t i = 0; i < sites.size(); ++i) {
		std::vector<VertexData> cellVerts = verts;
		std::vector<uint32_t> cellIndices = indices;

		for (size_t j = 0; j < sites.size(); ++j) {
			if (i == j) { continue; }

			Vector3 normal = sites[j] - sites[i];
			float len = Math::Length(normal);
			// ほぼ同位置のサイトは飛ばす
			if (len < 1e-5f) { continue; } 
			normal = normal / len;

			Vector3 midpoint = (sites[i] + sites[j]) * 0.5f;
			float planeDist = Math::Dot(normal, midpoint);

			ClipResult clipped = ClipMeshByPlane(cellVerts, cellIndices, normal, planeDist);
			cellVerts = clipped.backVerts;
			cellIndices = clipped.backIndices;

			// 他のサイトに完全に削られた
			if (cellIndices.empty()) {
				break;
			}
		}

		if (!cellIndices.empty()) {
			outFragments.push_back({ std::move(cellVerts), std::move(cellIndices) });
		}
	}
}

AABB FractureInstance::ComputeBounds(const std::vector<VertexData>& verts) {
	AABB aabb;
	if (verts.empty()) { return aabb; }

	aabb.min = Vector3(verts[0].position.x, verts[0].position.y, verts[0].position.z);
	aabb.max = Vector3(verts[0].position.x, verts[0].position.y, verts[0].position.z);
	for (const auto& v : verts) {
		aabb.min.x = std::min(aabb.min.x, v.position.x);
		aabb.min.y = std::min(aabb.min.y, v.position.y);
		aabb.min.z = std::min(aabb.min.z, v.position.z);
		aabb.max.x = std::max(aabb.max.x, v.position.x);
		aabb.max.y = std::max(aabb.max.y, v.position.y);
		aabb.max.z = std::max(aabb.max.z, v.position.z);
	}
	return aabb;
}

std::vector<Vector3> FractureInstance::GenerateSpherePlaneNormals(int count) const {
	std::vector<Vector3> normals;
	normals.reserve(count);

	if (count <= 1) {
		normals.push_back(Vector3(0.0f, 1.0f, 0.0f));
		return normals;
	}

	const float goldenAngle = 3.14159265f * (3.0f - std::sqrt(5.0f));
	for (int i = 0; i < count; ++i) {
		float y = 1.0f - (static_cast<float>(i) / static_cast<float>(count - 1)) * 2.0f;
		float radiusAtY = std::sqrt(std::max(0.0f, 1.0f - y * y));
		float theta = goldenAngle * static_cast<float>(i);

		float x = std::cos(theta) * radiusAtY;
		float z = std::sin(theta) * radiusAtY;
		normals.push_back(Vector3(x, y, z));
	}
	return normals;
}

void FractureInstance::CarveImpactCrater(std::vector<VertexData>& verts, std::vector<uint32_t>& indices,
	const Vector3& impactPos, float craterRadius, int planeCount) {

	std::vector<Vector3> normals = GenerateSpherePlaneNormals(planeCount);

	for (const Vector3& n : normals) {
		// 球面上の接点を通り、外向き法線nを持つ平面
		Vector3 planePoint = impactPos + n * craterRadius;
		float planeDist = Math::Dot(n, planePoint);

		ClipResult clipped = ClipMeshByPlane(verts, indices, n, planeDist);

		// n方向を残す。ClipMeshByPlaneは切断面のキャップも自動で追加
		verts = clipped.frontVerts;
		indices = clipped.frontIndices;

		// クレーターがメッシュ全体を覆ってしまった
		if (indices.empty()) {
			break;
		}
	}
}

void FractureInstance::AddTriangle(std::vector<VertexData>& verts, std::vector<uint32_t>& indices, const VertexData v[3]) {
	uint32_t base = static_cast<uint32_t>(verts.size());
	verts.push_back(v[0]);
	verts.push_back(v[1]);
	verts.push_back(v[2]);
	indices.push_back(base);
	indices.push_back(base + 1);
	indices.push_back(base + 2);
}