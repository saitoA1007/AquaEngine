#pragma once
#include <d3d12.h>
#include<vector>
#include <unordered_map>
#include <wrl.h>

#include"TextureManager.h"

#include"PSO/Core/PSOManager.h"
#include"PSO/Core/DrawPSOData.h"

#include"Sprite.h"

namespace GameEngine {

	// モデルを描画するモード
	enum class RenderMode2D {
		Normal,
		Add,
	};

	class SpriteRenderer {
	public:
		SpriteRenderer() = default;
		~SpriteRenderer() = default;

		/// <summary>
		/// 静的初期化
		/// </summary>
		/// <param name="commandList"></param>
		static void StaticInitialize(ID3D12GraphicsCommandList* commandList, TextureManager* textureManager, PSOManager* psoManager);

		/// <summary>
		/// 描画前処理
		/// </summary>
		/// <param name="mode"></param>
		/// <param name="blendMode"></param>
		static void PreDraw(RenderMode2D mode);

		/// <summary>
		/// 描画処理
		/// </summary>
		/// <param name="sprite"></param>
		/// <param name="textureHandle"></param>
		static void Draw(const Sprite* sprite, const uint32_t& textureHandle = 1024);

	private:

		// コマンドリスト
		static ID3D12GraphicsCommandList* commandList_;

		// psoデータのリスト
		static std::unordered_map<RenderMode2D, DrawPsoData> psoList_;

		// テクスチャ
		static TextureManager* textureManager_;
	};
}
