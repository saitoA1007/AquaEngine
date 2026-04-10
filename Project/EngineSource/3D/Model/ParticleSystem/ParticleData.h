#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "Transform.h"
#include"Range.h"
#include<cstdint>

namespace GameEngine {

	// パーティクルの調整パラメータ
	struct ParticelEmitter {
		uint32_t spawnMaxCount = 1; // 出現する数
		float spawnCoolTime = 1.0f; // 発生する時間
		bool isLoop = true; // ループするかを判断する
		bool isBillBoard = false; // ビルボードを使用するか判断する
		float lifeTime = 1.0f; // 生存時間
		Vector3 fieldAcceleration = { 0.0f,0.0f,0.0f }; // 場の力
		Range3 velocityRange = { {0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }; // 速度の範囲
		Range3 posRange = { {0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }; // 発生位置の範囲
		Range3 scaleRange = { {1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f} };  // 大きさの範囲
		Range4 colorRange = { {1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f} }; // 色の範囲
	};

	// パーティクルデータ
	struct ParticleData {
		Transform transform; // srt要素
		Vector3 velocity; // 速度
		Vector4 color;  // 色
		float lifeTime; // 生存時間
		float currentTime; // 現在の生存時間
	};
}