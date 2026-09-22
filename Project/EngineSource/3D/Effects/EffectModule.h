#pragma once
#include "ParticleBehavior.h"

namespace GameEngine {

	class EffectModule {
	public:
		EffectModule(std::string name);
		~EffectModule() = default;

		// 初期化処理
		void Initialize();

		// 更新処理
		void Update();

		// 描画処理
		void Draw();

	private:
		// パーティクル
		ParticleBehavior particle_;

		// 開始時間
		float startTime_ = 0.0f;

		// 生存時間
		float maxLifeTime_ = 1.0f;
	};
}