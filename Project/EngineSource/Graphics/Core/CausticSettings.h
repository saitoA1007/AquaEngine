#pragma once
#include <cstdint>

namespace GameEngine {

	/// <summary>
	/// フォトンによる氷のコースティクスの設定
	/// </summary>
	struct CausticSettings {
		bool enabled = true;              // コースティクスを使用するか
		uint32_t photonResolution = 1024; // 氷の範囲へ撃つフォトンの縦横の本数
		float emitRange = 32.0f;          // 氷を探す範囲。カメラを中心にライトから見た半径
		uint32_t gatherRadius = 4;        // フォトンを集めるピクセルの半径
		float intensity = 1.0f;           // コースティクスの明るさ
		float iceShadowDensity = 0.8f;    // コースティクス使用時の氷の影の濃さ。透過光はフォトンが運ぶ
		bool debugView = false;           // コースティクスだけを表示する
	};
}
