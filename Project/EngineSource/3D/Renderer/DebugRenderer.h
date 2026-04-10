#pragma once
#include <d3d12.h>
#include<vector>

#include"Vector3.h"
#include"Vector4.h"
#include"Matrix4x4.h"
#include"PSO/Core/PSOManager.h"
#include"PSO/Core/DrawPSOData.h"

#include"Geometry.h"

namespace GameEngine {

	// 頂点データ
	struct VertexPosColor {
		Vector4 pos;   // xyz座標
		Vector4 color; // RGBA
	};

	class DebugRenderer final {
	public:

		struct LineData {
			Vector3 start;
			Vector3 end;
			Vector4 color;
		};

	public:
		DebugRenderer() = default;
		~DebugRenderer() = default;

		/// <summary>
		/// 静的初期化
		/// </summary>
		static void StaticInitialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, PSOManager* psoManager);

		/// <summary>
		/// インスタンス作成
		/// </summary>
		static std::unique_ptr<DebugRenderer> Create();

		/// <summary>
		/// クリアする
		/// </summary>
		void Clear();

		/// <summary>
		/// 線を追加
		/// </summary>
		void AddLine(const Vector3& start, const Vector3& end, const Vector4& color = { 0,1,1,1 });

		/// <summary>
		/// AABBボックスを追加
		/// </summary>
		void AddBox(const AABB& aabb, const Vector4& color = { 0,1,1,1 });

		/// <summary>
		/// AABBボックスを追加
		/// </summary>
		void AddBox(const Vector3& centerPos,const Vector3& size, const Vector4& color = { 0,1,1,1 });

		/// <summary>
		/// OBBボックスを追加
		/// </summary>
		void AddBox(const OBB& obb, const Vector4& color);

		/// <summary>
		/// 球を追加
		/// </summary>
		void AddSphere(const Sphere& sphere, const Vector4& color = { 0,1,1,1 }, int segments = 16);

		/// <summary>
		/// レイを追加
		/// </summary>
		void AddRay(const Segment segment, float length = 10.0f, const Vector4& color = { 0,1,1,1 });

		/// <summary>
		/// 2Dの円を追加
		/// </summary>
		void AddCircle(const Vector3& centerPos, const Vector3& normal, float radius, const Vector4& color = { 0,1,1,1 }, int segments = 16);

		/// <summary>
		/// すべての線を描画
		/// </summary>
		void DrawAll(const Matrix4x4& VPMatrix);

		/// <summary>
		/// デバッグ描画の有効化を設定
		/// </summary>
		void SetEnabled(bool enabled) { isEnabled_ = enabled; }
		bool IsEnabled() const { return isEnabled_; }

	private:
		DebugRenderer(const DebugRenderer&) = delete;
		DebugRenderer& operator=(const DebugRenderer&) = delete;

		// 静的メンバー
		static ID3D12Device* device_;
		static ID3D12GraphicsCommandList* commandList_;
		static DrawPsoData pso_;

		bool isEnabled_ = true;

		// ラインの数を保持
		std::vector<LineData> lines_;
		
		// 頂点情報
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		VertexPosColor* vertexData_ = nullptr;

		// 最大の頂点数
		uint32_t maxVertices_ = 10000;

		// トランスフォーム行列用
		Microsoft::WRL::ComPtr<ID3D12Resource> transformMatrixResource_;
		struct TransformMatrix {
			Matrix4x4 VP;
		};
		TransformMatrix* transformMatrixData_ = nullptr;

	private:
		/// <summary>
	    /// 描画前処理
	    /// </summary>
		void PreDraw();

		/// <summary>
		/// LineMeshを作成または更新
		/// </summary>
		void UpdateLineMeshes();
	};

}