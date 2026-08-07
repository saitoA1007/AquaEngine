#pragma once
#include "FractureInstance.h"

namespace GameEngine {

	/// <summary>
	/// 無傷、マクロ破片、マイクロ破片の3つの状態をまとめて保持する。
	/// </summary>
	class FractureBreakState {
	public:
		FractureInstance& Intact() { return intact_; }
		const FractureInstance& Intact() const { return intact_; }

		FractureInstance& MacroDebris() { return macroDebris_; }
		const FractureInstance& MacroDebris() const { return macroDebris_; }

		FractureInstance& MicroDebris() { return microDebris_; }
		const FractureInstance& MicroDebris() const { return microDebris_; }

		bool HasIntact() const { return intact_.HasInstances(); }
		bool HasMacroDebris() const { return macroDebris_.HasInstances(); }
		bool HasMicroDebris() const { return microDebris_.HasInstances(); }

	private:
		// 元の静的チャンク
		FractureInstance intact_;
		// 事前分割のまま切り離されて落ちる破片
		FractureInstance macroDebris_;
		// ランタイムカットされた破片
		FractureInstance microDebris_;
	};
}
