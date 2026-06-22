#include "ParticleUpdateModules.h"
#include "EasingManager.h"
using namespace GameEngine;

//==================================================
// 速度変化モジュール
//==================================================

void VelocityOverLifeTimeModule::Update(ParticleData& particleData, [[maybe_unused]] float time) {
	// 速度を補間
	particleData.velocity = Lerp(particleData.startSpeed, endVelocity_, particleData.currentTime);
}

//==================================================
// サイズ変化モジュール
//==================================================

void SizeOverLifeTimeModule::Update(ParticleData& particleData, [[maybe_unused]] float time) {

	if (separateAxes_) {
		particleData.transform.scale = Lerp(particleData.startSize, separateAxesEndSize_, particleData.currentTime);
	} else {
		particleData.transform.scale = Lerp(particleData.startSize, Vector3(endSize_, endSize_, endSize_), particleData.currentTime);
	}
}

//==================================================
// 透明度変化モジュール
//==================================================

void AlphaOverLifeTimeModule::Update(ParticleData& particleData, [[maybe_unused]] float time) {
	particleData.color.w = Lerp(particleData.startColor.w, endAlpha_, particleData.currentTime);
}

//==================================================
// 引力モジュール
//==================================================

void AttractionModule::Update(ParticleData& particleData, float time) {
	// 目標位置へのベクトルを計算
	Vector3 toTarget = targetPos_ - particleData.transform.translate;
	float distanceSquared = toTarget.LengthSquared();

	if (distanceSquared > 0.0001f) {
		Vector3 direction = toTarget.Normalize();

		// 目標に向かう加速度を現在の速度に加算
		particleData.velocity += direction * strength_ * time;
	}

	// 速度の減衰
	particleData.velocity = particleData.velocity * (1.0f - damping_ * time);
}

//==================================================
// らせんモジュール
//==================================================

void VortexModule::Update(ParticleData& particleData, float time) {
	// 中心点からパーティクルへのベクトル
	Vector3 offset = particleData.transform.translate - centerPos_;
	offset.y = 0.0f;

	// Y軸まわりの回転を計算するため、XZ平面上での距離を測る
	float distanceXZ = offset.Length();

	if (distanceXZ > 0.0001f) {
		// 回転方向のベクトルを計算
		Vector3 tangent;
		tangent.x = -offset.z / distanceXZ;
		tangent.y = 0.0f;
		tangent.z = offset.x / distanceXZ;

		// 中心に向かうベクトルを計算
		Vector3 inward;
		inward.x = -offset.x / distanceXZ;
		inward.y = 0.0f;
		inward.z = -offset.z / distanceXZ;

		// 目標速度
		Vector3 targetVelocity = { 0.0f, 0.0f, 0.0f };

		// 回転速度を適用
		targetVelocity += tangent * rotationSpeed_;

		// 吸い込み
		targetVelocity += inward * attractionSpeed_;

		// 軸方向の速度を適用
		targetVelocity.y = axisSpeed_;

		// 現在の速度から目標の渦速度へ徐々に近づける
		particleData.velocity += targetVelocity * time;
	}
}

//==================================================
// 速度方向に回転させるモジュール
//==================================================

void RotationByVelocityModule::Update(ParticleData& particleData, [[maybe_unused]] float time) {
	Vector3 vel = particleData.velocity;
	float lenSq = vel.x * vel.x + vel.y * vel.y + vel.z * vel.z;

	// 速度がほぼ0のときは、直前の向きを維持す
	if (lenSq > 0.0001f) {
		float len = std::sqrt(lenSq);
		Vector3 dir = { vel.x / len, vel.y / len, vel.z / len };

		// 横方向の回転（Y軸まわりの回転: Yaw）
		float yaw = std::atan2(dir.x, dir.z);

		// 縦方向の回転
		float pitch = -std::asin(dir.y);

		// パーティクルの回転に適用
		particleData.transform.rotate = { pitch, yaw, 0.0f };
	}
}