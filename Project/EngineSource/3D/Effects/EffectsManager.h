#pragma once
#include <string>
#include <unordered_map>
#include "EffectObject.h"

namespace GameEngine {

	class EffectsManager {
	public:

		void Initialize();

		void Update();

	public:

		/// <summary>
		/// エフェクトを登録
		/// </summary>
		/// <param name="effectName">登録するエフェクトの名前</param>
		void RegisterEffect(std::string effectName);

		/// <summary>
		/// モデルの名前からエフェクトを取得
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		EffectObject GetEffectByName(const std::string& name) const;

	private:

		// エフェクトのデータ
		std::unordered_map<std::string, EffectObject> dataList_;
	};
}
