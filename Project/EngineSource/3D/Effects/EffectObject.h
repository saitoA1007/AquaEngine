#pragma once
#include <vector>
#include "EffectModule.h"

namespace GameEngine {

	class EffectObject {
	public:
		EffectObject(std::string name);

	private:
		// エフェクトの名前
		std::string name_;

		// エフェクトデータ
		std::vector<EffectModule> effectData_;
	};
}

