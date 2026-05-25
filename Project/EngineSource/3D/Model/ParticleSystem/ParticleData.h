#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "Transform.h"
#include <cstdint>

namespace GameEngine {

	// パーティクルデータ
	struct ParticleData {
		Transform transform; // srt要素
		Vector3 velocity; // 速度
		Vector4 color;  // 色
		Vector4 startColor;
		float lifeTime; // 生存時間
		float currentTime; // 現在の生存時間

		Vector3 startSize;
		Vector3 startSpeed;

		bool IsAlive() const { return lifeTime <= currentTime; }
	};
}