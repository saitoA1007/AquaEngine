#include "ParticleEmitModules.h"
#include "TextureManager.h"
#include "RandomGenerator.h"
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
