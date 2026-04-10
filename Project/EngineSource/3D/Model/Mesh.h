#pragma once
#include <d3d12.h>
#include <wrl.h>

#include"VertexData.h"

//==================================================================
// 
// デバック用のグリッド生成システムだけここじゃない感強いので後で変更する。
// それか削除してもいいかもしれない
//
//==================================================================

namespace GameEngine {

	class Mesh final {
	public:
		Mesh() = default;
		~Mesh();

		/// <summary>
		/// 三角形の平面メッシュを作成
		/// </summary>
		/// <param name="device">デバイス</param>
		void CreateTrianglePlaneMesh(ID3D12Device* device);

		/// <summary>
		/// グリッド平面のメッシュを作成
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="size">x:横幅,y:縦幅</param>
		void CreateGridPlaneMesh(ID3D12Device* device, const Vector2& size);

		/// <summary>
		/// 平面のメッシュを作成
		/// </summary>
		/// <param name="device"></param>
		/// <param name="size"></param>
		void CreatePlaneMesh(ID3D12Device* device, const Vector2& size);

		/// <summary>
		/// 球のメッシュを作成
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="subdivision">分割数</param>
		void CreateSphereMesh(ID3D12Device* device, uint32_t subdivision);

		/// <summary>
		/// モデルデータを読み込んでメッシュを作成する
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="modelData">読み込んだモデルデータ</param>
		void CreateModelMesh(ID3D12Device* device,ModelData modelData,const uint32_t& index);

		/// <summary>
		/// リングのメッシュを作成
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="subdivision">分割数</param>
		/// <param name="outerDiameter">外径</param>
		/// <param name="innerDiameter">内径</param>
		void CreateRingMesh(ID3D12Device* device,const uint32_t& subdivision,const float& outerRadius,const float& innerRadius);

		/// <summary>
		/// 円柱のメッシュを作成
		/// </summary>
		/// <param name="device"></param>
		/// <param name="subdivision"></param>
		/// <param name="topRadius"></param>
		/// <param name="bottomRadius"></param>
		/// <param name="height"></param>
		void CreateCylinderMesh(ID3D12Device* device, const uint32_t& subdivision, const float& topRadius, const float& bottomRadius, const float& height);

	public:

		/// <summary>
		/// 頂点バッファビューを取得
		/// </summary>
		/// <returns></returns>
		const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBufferView_; }

		/// <summary>
		/// インデックスバッファビューを取得
		/// </summary>
		/// <returns></returns>
		const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBufferView_; }

		/// <summary>
		/// 全ての頂点数を取得
		/// </summary>
		/// <returns></returns>
		const uint32_t GetTotalVertices()const { return totalVertices_; }

		/// <summary>
		/// 全てのインデックス数を取得
		/// </summary>
		/// <returns></returns>
		const uint32_t GetTotalIndices()const { return totalIndices_; }

		/// <summary>
		/// メッシュに対応するマテリアル名を取得
		/// </summary>
		/// <returns></returns>
		const std::string GetMaterialName() const { return materialName_; }

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

		// メッシュデータ
		VertexData* vertexData_ = nullptr;

		uint32_t totalVertices_ = 0;
		uint32_t totalIndices_ = 0;

		// マテリアル名
		std::string materialName_ = "default";
	};
}