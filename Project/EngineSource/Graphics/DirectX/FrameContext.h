#pragma once
#include <cstdint>

namespace GameEngine {

	// 同時に処理中にできるフレーム数
	inline constexpr uint32_t kFrameCount = 2;

	/// <summary>
	/// 現在CPUが記録しているフレームのスロット番号を管理する
	/// </summary>
	class FrameContext {
	public:
		// 現在のフレームスロットを取得
		static uint32_t GetFrameIndex() { return frameIndex_; }

		// フレームスロットを設定
		static void SetFrameIndex(uint32_t frameIndex) { frameIndex_ = frameIndex; }

	private:
		static inline uint32_t frameIndex_ = 0;
	};
}
