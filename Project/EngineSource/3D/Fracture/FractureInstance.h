#pragma once
#include "Matrix4x4.h"
#include "Transform.h"
#include "ConstantBuffer.h"
#include "StructuredBuffer.h"
#include "PackedGeometryBuffer.h"
#include "RuntimeFractureBuffer.h"

namespace GameEngine {

	// CPU側で各破片の状態を管理
	struct FractureChunkState {
		Transform transform;
		Vector3 velocity;
	};

	struct FractureForGPU {
		Matrix4x4 world;
		Matrix4x4 worldInverseTranspose;

		uint32_t vertexOffset; // PackedGeometryBuffer内でのチャンクの頂点開始位置
		uint32_t indexOffset; // PackedGeometryBuffer内でのチャンクのインデックス開始位置
		uint32_t indexCount; // IndexCountPerInstance に相当
		uint32_t chunkId; // シェーダー側で gParticle を引くためのID
	};

	struct FractureIndirectCommand {
		uint32_t instanceIndex;
		D3D12_DRAW_INDEXED_ARGUMENTS drawArguments;
	};

	struct ClipResult {
		std::vector<VertexData> frontVerts;
		std::vector<VertexData> backVerts;
		std::vector<uint32_t> frontIndices;
		std::vector<uint32_t> backIndices;
	};

	/// <summary>
	/// 複数描画用のワールド行列
	/// </summary>
	class FractureInstance {
	public:
		FractureInstance() = default;
		~FractureInstance();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="transform"></param>
		void Initialize(const std::vector<uint32_t>& chunkIds, const PackedGeometryBuffer& geometryBuffer);

		/// <summary>
		/// SRTを適応
		/// </summary>
		void Update();

	public:

		// GeometryRangeを直接渡して初期化（ランタイムカットの結果など、PackedGeometryBuffer以外の出所に使う）
		void InitializeFromRanges(const std::vector<GeometryRange>& ranges);

		// 指定メッシュを衝撃点周りでランタイムカットし、その結果でこのインスタンスを構築する
		void ApplyRuntimeCut(const Fragment& source, const Vector3& impactPos, int numSites);

		const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetInstancingSrvGPU() const { return buffer_.GetSrvGpuHandle(); }

		/// <summary>
		/// 描画するモデルの数
		/// </summary>
		/// <returns></returns>
		const uint32_t GetNumInstance() { return numInstance_; }

		// トランスフォーム
		std::vector<FractureChunkState>& GetTransformDatas() { return transformData_; }

		StructuredBuffer<FractureForGPU>& GetBuffer() { return buffer_; }

		ConstantBuffer<FractureIndirectCommand>& GetArgumentBuffer() { return argumentBuffer_; }

		// ランタイムカットで構築された場合のみ有効
		bool HasRuntimeGeometry() const { return runtimeBuffer_ != nullptr; }
		const D3D12_VERTEX_BUFFER_VIEW& GetRuntimeVertexBufferView() const { return runtimeBuffer_->GetVertexBufferView(); }
		const D3D12_INDEX_BUFFER_VIEW& GetRuntimeIndexBufferView() const { return runtimeBuffer_->GetIndexBufferView(); }

	private:
		// コピー禁止
		FractureInstance(const FractureInstance&) = delete;
		FractureInstance& operator=(const FractureInstance&) = delete;

		// インスタンスが持つsrvインデックス
		uint32_t srvIndex_ = 0;

		// transformData配列数
		uint32_t numInstance_ = 0;

		// リソース
		StructuredBuffer<FractureForGPU> buffer_;
		FractureForGPU* instancingData_ = nullptr;

		std::vector<FractureChunkState> transformData_;

		// ExecuteIndirect用の間接描画引数バッファ
		ConstantBuffer<FractureIndirectCommand> argumentBuffer_;
		FractureIndirectCommand* argumentData_ = nullptr;

		// これ未満の三角形数になった破片はそれ以上分割しない
		size_t kMinTriangleCount = 12;

		// ランタイム分割
		std::unique_ptr<RuntimeFractureBuffer> runtimeBuffer_;

	private:

		void AllocateBuffers(uint32_t count);
		void WriteInstance(uint32_t index, const GeometryRange& range, uint32_t chunkId);

		// 平面によるメッシュクリッピング
		ClipResult ClipMeshByPlane(const std::vector<VertexData>& verts,
			const std::vector<uint32_t>& indices,
			const Vector3& planeNormal, float planeDist);

		// 平面をまたぐ三角形の分割
		void SplitStraddlingTriangle(const VertexData v[3], const float d[3],
			const Vector3& planeNormal, float planeDist,
			ClipResult& result, std::vector<std::pair<VertexData, VertexData>>& cutEdges);

		// 切断エッジを繋いでキャップ面を生成
		void CapCutFace(const std::vector<std::pair<VertexData, VertexData>>& cutEdges,
			const Vector3& planeNormal, ClipResult& result);

		std::vector<Vector3> GenerateVoronoiSites(const AABB& bounds, const Vector3& impactPos, int numSites) const;

		void VoronoiFracture(const std::vector<VertexData>& verts, const std::vector<uint32_t>& indices,
			const Vector3& impactPos, int numSites, std::vector<Fragment>& outFragments);

		AABB ComputeBounds(const std::vector<VertexData>& verts);

		// 三角形追加
		static void AddTriangle(std::vector<VertexData>& verts, std::vector<uint32_t>& indices, const VertexData v[3]);
	};
}

