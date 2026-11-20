#pragma once
#include <cstdint>

namespace GameEngine {

	/// <summary>
	/// レイトレースによるアンビエントオクルージョンの設定
	/// </summary>
	struct RtaoSettings {
		bool enabled = true;          // AOを使用するか
		uint32_t sampleCount = 2;     // 1ピクセルあたりのAOレイの本数
		float radius = 1.5f;          // 遮蔽物を探す距離
		float strength = 1.0f;        // AOの強さ
		float ambientIntensity = 0.3f; // 背景のキューブマップから受ける環境光の強さ
		bool debugView = false;       // AOだけを白黒で表示する
		bool historyClamp = true;     // 履歴を今フレームの周囲の値の範囲に収める
		float clampGamma = 1.0f;      // 収める範囲の広さ
	};
}
