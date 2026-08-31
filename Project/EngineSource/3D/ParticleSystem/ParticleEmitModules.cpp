#include "ParticleEmitModules.h"
#include "TextureManager.h"
#include "RandomGenerator.h"
#include "MyMath.h"
#include <numbers>
using namespace GameEngine;

//==================================================
// テクスチャモジュール
//==================================================

void TextureModule::SetTexture(TextureManager* textureManager) {
	textureData_.handle = textureManager->GetHandleByName(textureData_.name);
}

//==================================================
// 速度モジュール
//==================================================

void VelocityEmitModule::Create(ParticleData& particleData) {
	particleData.velocity = {
		RandomGenerator::Get(velocityRange_.min.x, velocityRange_.max.x),
		RandomGenerator::Get(velocityRange_.min.y, velocityRange_.max.y),
		RandomGenerator::Get(velocityRange_.min.z, velocityRange_.max.z),
	};
}

//==================================================
// 方向指定速度モジュール
//==================================================

void DirectionEmitModule::Create(ParticleData& particleData) {
	// 基準方向を正規化
	Vector3 baseDir = direction_;
	if (baseDir.LengthSquared() <= 0.0001f) {
		baseDir = { 0.0f, 0.0f, 1.0f };
	} else {
		baseDir = baseDir.Normalize();
	}

	if (spreadAngle_ > 0.0f) {
		// 基準方向と平行にならないup軸を選ぶ
		Vector3 up = (std::fabs(Math::Dot(baseDir, Vector3(0.0f, 1.0f, 0.0f))) > 0.99f) ?
			Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f);
		Vector3 perpendicular = Math::Normalize(Math::Cross(baseDir, up));

		// 基準方向を軸に回転させ、広がりの方向を決める
		float spinAngle = RandomGenerator::Get(0.0f, 2.0f * std::numbers::pi_v<float>);
		Quaternion spin = Math::MakeRotateAxisAngleQuaternion(baseDir, spinAngle);
		Vector3 spreadAxis = Math::RotateVector(perpendicular, spin);

		// spreadAxisを軸に基準方向をばらつき角の範囲で傾ける
		float tiltAngle = RandomGenerator::Get(0.0f, spreadAngle_ * (std::numbers::pi_v<float> / 180.0f));
		Quaternion tilt = Math::MakeRotateAxisAngleQuaternion(spreadAxis, tiltAngle);
		baseDir = Math::RotateVector(baseDir, tilt);
	}

	float speed = RandomGenerator::Get(minSpeed_, maxSpeed_);
	particleData.velocity = baseDir * speed;
}

//==================================================
// 回転モジュール
//==================================================

void RotateEmitModule::Create(ParticleData& particleData) {
	particleData.transform.rotate = {
		RandomGenerator::Get(rotateRange_.min.x, rotateRange_.max.x),
		RandomGenerator::Get(rotateRange_.min.y, rotateRange_.max.y),
		RandomGenerator::Get(rotateRange_.min.z, rotateRange_.max.z),
	};
}

//==================================================
// サイズモジュール
//==================================================

void ScaleEmitModule::Create(ParticleData& particleData) {
	if (separateAxes_) {
		particleData.transform.scale = {
		RandomGenerator::Get(scaleRange_.min.x, scaleRange_.max.x),
		RandomGenerator::Get(scaleRange_.min.y, scaleRange_.max.y),
		RandomGenerator::Get(scaleRange_.min.z, scaleRange_.max.z),
		};
	} else {
		float randomScale = RandomGenerator::Get(scaleRange_.min.x, scaleRange_.max.x);
		particleData.transform.scale = { randomScale, randomScale, randomScale };
	}

	particleData.startSize = particleData.transform.scale;
}

//==================================================
// 回転速度モジュール
//==================================================

void RotateVelocityEmitModule::Create(ParticleData& particleData) {
	particleData.rotateVelocity = {
		RandomGenerator::Get(velocityRange_.min.x, velocityRange_.max.x),
		RandomGenerator::Get(velocityRange_.min.y, velocityRange_.max.y),
		RandomGenerator::Get(velocityRange_.min.z, velocityRange_.max.z),
	};
}

//==================================================
// 発射形状モジュール
//==================================================

void ShapeEmitModule::Create(ParticleData& particleData) {
	Vector3 centerPos = particleData.transform.translate;

	switch (emitterShape_.type) {
	case EmitShapeType::Sphere: {
		Vector3 randomDir;
		while (true) {
			// 立方体の中でランダムに点を取り、球の中に入るまで繰り返す
			randomDir = RandomGenerator::GetVector3(Vector3(-1, -1, -1), Vector3(1, 1, 1));
			if (randomDir.LengthSquared() <= 1.0f && randomDir.LengthSquared() > 0.0001f) {
				break;
			}
		}

		if (emitterShape_.emitFromShell) {
			// 表面
			particleData.transform.translate = centerPos + randomDir.Normalize() * emitterShape_.radius;
		} else {
			// 内部
			particleData.transform.translate = centerPos + randomDir * emitterShape_.radius;
		}
		break;
	}

	case EmitShapeType::Hemisphere: {
		Vector3 randomDir;
		while (true) {
			randomDir = RandomGenerator::GetVector3(Vector3(-1, -1, -1), Vector3(1, 1, 1));
			// 阪急
			if (randomDir.LengthSquared() <= 1.0f && randomDir.LengthSquared() > 0.0001f && randomDir.y >= 0.0f) {
				break;
			}
		}

		if (emitterShape_.emitFromShell) {
			// 表面
			particleData.transform.translate = centerPos + randomDir.Normalize() * emitterShape_.radius;
		} else {
			// 内部
			particleData.transform.translate = centerPos + randomDir * emitterShape_.radius;
		}
		break;
	}

	case EmitShapeType::Box: {
		Vector3 half = emitterShape_.boxSize * 0.5f;
		particleData.transform.translate = RandomGenerator::GetVector3(centerPos - half, centerPos + half);
		break;
	}
	}
}

//==================================================
// 色モジュール
//==================================================

void ColorEmitModule::Create(ParticleData& particleData) {
	particleData.color = {
		RandomGenerator::Get(minColor_.x, maxColor_.x),
		RandomGenerator::Get(minColor_.y, maxColor_.y),
		RandomGenerator::Get(minColor_.z, maxColor_.z),
		RandomGenerator::Get(minColor_.w, maxColor_.w)
	};
	particleData.startColor = particleData.color;
}
