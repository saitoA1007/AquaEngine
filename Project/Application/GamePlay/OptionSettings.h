#pragma once
#include <cstdint>

/// <summary>
/// オプションで変更できる設定
/// </summary>
struct OptionSettings {

	// 音量の段階数
	static constexpr int32_t kMaxVolumeLevel = 10;

	// BGMの音量段階
	int32_t bgmVolumeLevel = kMaxVolumeLevel;
	// SEの音量段階
	int32_t seVolumeLevel = kMaxVolumeLevel;

	// 視点の左右操作を反転するか
	bool isInvertCameraX = false;
	// 視点の上下操作を反転するか
	bool isInvertCameraY = false;

	// 音量段階を0.0~1.0の音量に変換
	static float ToVolume(int32_t level) {
		return static_cast<float>(level) / static_cast<float>(kMaxVolumeLevel);
	}
};
