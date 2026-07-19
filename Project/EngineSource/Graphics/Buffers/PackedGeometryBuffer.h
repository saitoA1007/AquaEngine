#pragma once
#include <cassert>
#include <unordered_map>
#include <vector>
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexData.h"
#include "FractureData.h"

namespace GameEngine {

	/// <summary>
	/// 破壊オブジェクト1つ分の全チャンクを、1本の共有頂点、インデックスバッファにまとめて保持する
	/// </summary>
	class PackedGeometryBuffer final {
	public:
		PackedGeometryBuffer() = default;
		~PackedGeometryBuffer() = default;

		/// <summary>
		/// 同じ破壊グループに属するチャンク群を1本のバッファに詰め込む
		/// </summary>
		/// <param name="chunkMeshes">fractureInfoを持つMeshDataの一覧</param>
		void Build(const std::vector<MeshData>& chunkMeshes) {

			std::vector<VertexData> packedVertices;
			std::vector<uint32_t> packedIndices;

			for (const auto& meshData : chunkMeshes) {
				assert(meshData.fractureInfo.has_value() && "PackedGeometryBufferにはfractureInfoを持つMeshDataのみ渡してください");

				GeometryRange range;
				range.vertexOffset = static_cast<uint32_t>(packedVertices.size());
				range.vertexCount = static_cast<uint32_t>(meshData.vertices.size());
				range.indexOffset = static_cast<uint32_t>(packedIndices.size());
				range.indexCount = static_cast<uint32_t>(meshData.indices.size());

				// インデックス値はチャンク内で完結したローカル参照
				packedVertices.insert(packedVertices.end(), meshData.vertices.begin(), meshData.vertices.end());
				packedIndices.insert(packedIndices.end(), meshData.indices.begin(), meshData.indices.end());

				rangesByChunkId_[meshData.fractureInfo->chunkId] = range;
			}
			
			vertexBuffer_.Create(packedVertices);
			indexBuffer_.Create(packedIndices);
		}

		// 指定チャンクの描画範囲を取得
		const GeometryRange& GetRange(uint32_t chunkId) const {
			auto it = rangesByChunkId_.find(chunkId);
			assert(it != rangesByChunkId_.end() && "指定されたchunkIdのGeometryRangeが見つかりません");
			return it->second;
		}

		// バッファは1本しかないため、ビューは全チャンクで共有する
		const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBuffer_.GetView(); }
		const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBuffer_.GetView(); }

		uint32_t GetVertexBufferSrvIndex() const { return vertexBuffer_.GetSrvIndex(); }
		uint32_t GetIndexBufferSrvIndex() const { return indexBuffer_.GetSrvIndex(); }

	private:
		VertexBuffer<VertexData> vertexBuffer_;
		IndexBuffer indexBuffer_;

		// このバッファ内での描画範囲
		std::unordered_map<uint32_t, GeometryRange> rangesByChunkId_;
	};
}