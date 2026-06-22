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
		Vector3 dir; // 方向

		Vector3 startSize;
		Vector3 startSpeed;

		bool IsAlive() const { return 1.0f <= currentTime; }
	};

	// 形状
	enum class EmitShapeType {
		Point,       // 点
		Sphere,      // 球
		Hemisphere,  // 半球
		Box,         // 直方体
	};
	inline constexpr const char* EmitShapeTypeNames[] = {
		"Point", "Sphere", "Hemisphere", "Box"
	};
	inline constexpr int kEmitShapeTypeCount = 4;

	struct EmitterShape {
		EmitShapeType type = EmitShapeType::Point;

		// Sphere、Hemisphere
		float radius = 1.0f;
		bool  emitFromShell = false; // 表面からのみ発射

		// Box
		Vector3 boxSize = { 1.0f, 1.0f, 1.0f };
	};
}