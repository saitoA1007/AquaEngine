#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "Transform.h"
#include <cstdint>
#include <string>

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
		uint32_t textureHandle = 0; // テクスチャ

		Vector3 startSize;
		Vector3 startSpeed;
		Vector3 rotateVelocity; // 回転速度

		bool IsAlive() const { return 1.0f <= currentTime; }
	};

	// GPU用のパーティクルデータ
	struct ParticleCS
	{
		Vector3 translate;
		Vector3 scale;
		float lifeTime;
		Vector3 velocity;
		float currentTime;
		Vector4 color;
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

	// テクスチャデータ
	struct TextureData {
		uint32_t handle = 0;
		std::string name = "None";
	};
}