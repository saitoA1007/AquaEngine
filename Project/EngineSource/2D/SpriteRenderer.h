#pragma once
#include <d3d12.h>
#include <wrl.h>

#include "SrvManager.h"
#include "Sprite.h"

namespace GameEngine {

	
	class SpriteRenderer {
	public:
		SpriteRenderer() = default;
		~SpriteRenderer() = default;

		/// <summary>
		/// 静的初期化
		/// </summary>
		/// <param name="commandList"></param>
		static void StaticInitialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager);

		/// <summary>
		/// 描画処理
		/// </summary>
		/// <param name="sprite"></param>
		/// <param name="textureHandle"></param>
		static void Draw(const Sprite* sprite);

	private:

		// コマンドリスト
		static ID3D12GraphicsCommandList* commandList_;

		// テクスチャ
		static SrvManager* srvManager_;
	};
}
