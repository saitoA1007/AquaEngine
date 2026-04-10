#pragma once
#include <random>
#include"Vector3.h"

class RandomGenerator {
public:

	/// <summary>
	/// 初期化処理(シーンで1回だけ処理)
	/// </summary>
	static void Initialize();

	// 指定した型のランダム値を取得
	template <typename T>
	static T Get(T min, T max);

	static Vector3 GetVector3(float min, float max);

private:

	static std::mt19937 randomEngine_;
};