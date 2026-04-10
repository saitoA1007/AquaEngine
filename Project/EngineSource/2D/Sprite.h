#pragma once
#include <d3d12.h>
#include<iostream>
#include <wrl.h>
#include"Matrix4x4.h"
#include"Vector2.h"
#include"Vector4.h"
#include"Transform.h"

namespace GameEngine {

	class Sprite final {
	public:

		// 頂点データ
		struct VertexPosUv {
			Vector4 position;
			Vector2 texcoord;
		};

		// 定数バッファ
		struct ConstBufferData {
			Vector4 color;
			Matrix4x4 uvTransform;
			Matrix4x4 WVP;
		};

	public:
		Sprite() = default;
		~Sprite();

		/// <summary>
		/// 静的初期化
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="window_width">画面幅</param>
		/// <param name="window_height">画面高さ</param>
		static void StaticInitialize(ID3D12Device* device, int32_t width, int32_t height);

		/// <summary>
		/// スプライト生成
		/// </summary>
		/// <param name="position">座標</param>
		/// <param name="size">サイズ</param>
		/// <param name="anchorPoint">アンカーポイント</param>
		/// <param name="color">色</param>
		/// <param name="leftTop">画像の描画する左上の位置</param>
		/// <param name="textureSize">画像の描画したい範囲</param>
		/// <param name="textureMaxSize">画像のサイズ</param>
		/// <returns></returns>
		static std::unique_ptr<Sprite> Create(const Vector2& position,const Vector2& size,const Vector2& anchorPoint,const Vector4& color = { 1, 1, 1, 1 },
			const Vector2& leftTop={0.0f,0.0f}, const Vector2& textureSize={1.0f,1.0f}, const Vector2& textureMaxSize={1.0f,1.0f});

	public:

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update();

		/// <summary>
		/// 座標の設定
		/// </summary>
		/// <param name="position">座標</param>
		void SetPosition(const Vector2& position);

		/// <summary>
		/// サイズの設定
		/// </summary>
		/// <param name="size">サイズ</param>
		void SetSize(const Vector2& size);

		/// <summary>
		/// 色の設定
		/// </summary>
		/// <param name="color">色</param>
		void SetColor(const Vector4& color);

		/// <summary>
		/// 透明度の設定
		/// </summary>
		/// <param name="alpha"></param>
		void SetAlpha(const float& alpha) { constBufferData_->color.w = alpha; }

		/// <summary>
		/// uvMatrixの設定
		/// </summary>
		/// <param name="transform"></param>
		void SetUvMatrix(const Transform& transform);

		const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBufferView_; }
		const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBufferView_; }

		ID3D12Resource* GetResource() const { return constBufferResource_.Get(); }

	public: // 変数

		// 座標
		Vector2 position_{};
		// 回転
		float rotate_ = 0.0f;
		// スケール
		Vector2 scale_ = { 1.0f,1.0f };
		// スプライト幅、高さ
		Vector2 size_ = { 100.0f, 100.0f };

		// 色
		Vector4 color_;

		Vector2 textureLeftTop_ = { 0.0f,0.0f };
		Vector2 textureSize_ = { 100.0f,100.0f };
		Vector2 textureMaxeSize_ = {};

	private:

		// デバイス
		static ID3D12Device* device_;
		// 射影行列
		static Matrix4x4 orthoMatrix_;
		
		// アンカーポイント
		Vector2 anchorPoint_{};
		// ワールド行列
		Matrix4x4 worldMatrix_;

		// 頂点バッファビューを作成する
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		// Sprite用の頂点リソース
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		VertexPosUv* vertexData_ = nullptr;

		// 頂点バッファビューを作成する
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		// Sprite用の頂点インデックスのリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

		// Sprite用のマテリアルリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> constBufferResource_;
		// マテリアルにデータを書き込む
		ConstBufferData* constBufferData_ = nullptr;

	private:

		/// <summary>
		/// メッシュを作成
		/// </summary>
		void CreateMesh();

		/// <summary>
		/// マテリアルを作成
		/// </summary>
		/// <param name="color"></param>
		void CreateConstBufferData(const Vector4& color);
	};
}
