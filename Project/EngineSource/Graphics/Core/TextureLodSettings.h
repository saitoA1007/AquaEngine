#pragma once

namespace GameEngine {

	/// <summary>
	/// レイトレーシングでテクスチャをサンプリングするときのミップレベルの設定
	/// </summary>
	struct TextureLodSettings {
		bool enabled = true; // LODを計算するか
		float bias = 0.0f;   // ミップレベルに足す値
	};
}
