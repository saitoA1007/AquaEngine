#pragma once
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexData.h"

namespace GameEngine {

	class Mesh final {
	public:
		Mesh() = default;
		~Mesh() = default;

		/// <summary>
		/// 三角形の平面メッシュを作成
		/// </summary>
		void CreateTrianglePlaneMesh();

		/// <summary>
		/// グリッド平面のメッシュを作成
		/// </summary>
		/// <param name="size">x:横幅,y:縦幅</param>
		void CreateGridPlaneMesh(const Vector2& size);

		/// <summary>
		/// 平面のメッシュを作成
		/// </summary>
		/// <param name="size"></param>
		void CreatePlaneMesh(const Vector2& size);

		/// <summary>
		/// 球のメッシュを作成
		/// </summary>
		/// <param name="subdivision">分割数</param>
		void CreateSphereMesh(uint32_t subdivision);

		/// <summary>
		/// モデルデータを読み込んでメッシュを作成する
		/// </summary>
		/// <param name="modelData">読み込んだモデルデータ</param>
		void CreateModelMesh(ModelData modelData,const uint32_t& index);

	public: // ゲッター

		// 頂点バッファビューを取得
		const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBuffer_.GetView(); }
		// インデックスバッファビューを取得
		const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBuffer_.GetView(); }
		// 全ての頂点数を取得
		uint32_t GetTotalVertices() const { return vertexBuffer_.GetTotalVertices(); }
		// 全てのインデックス数を取得
		uint32_t GetTotalIndices() const { return indexBuffer_.GetTotalIndices(); }
		// メッシュに対応するマテリアル名を取得
		const std::string& GetMaterialName() const { return materialName_; }

	private:
		VertexBuffer<VertexData> vertexBuffer_;
		IndexBuffer indexBuffer_;

		std::string materialName_ = "default";
	};
}