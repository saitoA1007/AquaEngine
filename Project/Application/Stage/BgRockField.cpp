#include "pch.h"
#include "BgRockField.h"
using namespace GameEngine;

BgRockField::BgRockField(GameEngine::Model* iceRockModel, GameEngine::Model* rockModel) {
	iceRockModel_ = iceRockModel;
	rockModel_ = rockModel;

	// 氷のマテリアルの初期値
	iceMaterial_.materialData_->color = { 0.81f,0.77f,0.92f,1.0f };

	// 輪ごとの初期値
	rings_[0] = { 16, 28.0f, 2.0f, { 0.4f,0.9f }, -0.3f, 0.6f };
	rings_[1] = { 14, 38.0f, 3.0f, { 0.8f,1.6f }, -0.8f, 0.4f };
	rings_[2] = { 12, 52.0f, 4.0f, { 1.6f,3.0f }, -1.5f, 0.3f };

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("BgRockField");
	RegisterParameter();
	debugParame_->Apply();

	// 岩を配置
	Generate();
}

void BgRockField::Initialize() {

}

void BgRockField::Update() {
	// 値が変更されたら配置し直す
	if (debugParame_->ApplyIfDirty()) {
		Generate();
	}
}

void BgRockField::Draw() {
	// 氷の岩を描画
	for (uint32_t i = 0; i < activeIceRockNum_; ++i) {
		iceRocks_[i]->DrawRaytracing(renderQueue_);
	}
	// 普通の岩を描画
	for (uint32_t i = 0; i < activeRockNum_; ++i) {
		rocks_[i]->DrawRaytracing(renderQueue_);
	}
}

void BgRockField::RegisterParameter() {
	int index = 0;
	std::string subGroup = "Generater";
	debugParame_->Register("CenterPos", centerPos_, index++, subGroup);
	debugParame_->Register("Seed", seed_, index++, subGroup);
	debugParame_->Register("MaxTilt", maxTilt_, index++, subGroup);
	debugParame_->Register("HeightScaleRange", heightScaleRange_, index++, subGroup);

	// 輪ごとの設定
	for (uint32_t i = 0; i < kRingNum; ++i) {
		subGroup = "Ring" + std::to_string(i);
		index = 0;
		debugParame_->Register("Count", rings_[i].count, index++, subGroup);
		debugParame_->Register("Radius", rings_[i].radius, index++, subGroup);
		debugParame_->Register("RadiusJitter", rings_[i].radiusJitter, index++, subGroup);
		debugParame_->Register("ScaleRange", rings_[i].scaleRange, index++, subGroup);
		debugParame_->Register("HeightOffset", rings_[i].heightOffset, index++, subGroup);
		debugParame_->Register("IceRatio", rings_[i].iceRatio, index++, subGroup);
	}

	// 見た目
	subGroup = "Material";
	index = 0;
	debugParame_->Register("RockColor", rockColor_, index++, subGroup);
	debugParame_->Register("IceColor", iceMaterial_.materialData_->color, index++, subGroup);
	debugParame_->Register("bubbleScale", iceMaterial_.materialData_->bubbleScale, index++, subGroup);
	debugParame_->Register("bubbleDensity", iceMaterial_.materialData_->bubbleDensity, index++, subGroup);
	debugParame_->Register("rimColor", iceMaterial_.materialData_->rimColor, index++, subGroup);
	debugParame_->Register("rimIntensity", iceMaterial_.materialData_->rimIntensity, index++, subGroup);
	debugParame_->Register("rimPower", iceMaterial_.materialData_->rimPower, index++, subGroup);
}

void BgRockField::Generate() {
	activeIceRockNum_ = 0;
	activeRockNum_ = 0;

	// シード値を固定して、毎回同じ配置になるようにする
	std::mt19937 engine(seed_);
	auto random = [&engine](float min, float max) {
		if (min > max) { std::swap(min, max); }
		return std::uniform_real_distribution<float>(min, max)(engine);
	};

	const float kTwoPi = std::numbers::pi_v<float> * 2.0f;

	for (const RingParam& ring : rings_) {
		const uint32_t count = std::min(ring.count, kMaxCountPerRing);
		if (count == 0) { continue; }

		// 等分した角度。輪ごとに開始角度をずらして、内側と外側の岩が一直線に並ばないようにする
		const float step = kTwoPi / static_cast<float>(count);
		const float startAngle = random(0.0f, step);

		for (uint32_t i = 0; i < count; ++i) {
			// 等間隔から少しずらした位置に置く
			float angle = startAngle + step * static_cast<float>(i) + random(-0.35f, 0.35f) * step;
			float radius = ring.radius + random(-ring.radiusJitter, ring.radiusJitter);
			float scale = random(ring.scaleRange.x, ring.scaleRange.y);
			bool isIce = random(0.0f, 1.0f) < ring.iceRatio;

			ModelComponent* rock = AcquireRock(isIce);
			Transform& transform = rock->worldTransform_.transform_;
			transform.scale = { scale, scale * random(heightScaleRange_.x, heightScaleRange_.y), scale };
			transform.rotate = { random(-maxTilt_, maxTilt_), random(0.0f, kTwoPi), random(-maxTilt_, maxTilt_) };
			transform.translate = centerPos_ + Vector3(std::cosf(angle) * radius, ring.heightOffset, std::sinf(angle) * radius);

			if (!isIce) {
				rock->materialData_->color = rockColor_;
			}
			rock->Update();
		}
	}
}

GameEngine::ModelComponent* BgRockField::AcquireRock(bool isIce) {
	auto& pool = isIce ? iceRocks_ : rocks_;
	uint32_t& activeNum = isIce ? activeIceRockNum_ : activeRockNum_;

	// 足りなければ生成する
	if (activeNum >= pool.size()) {
		auto rock = std::make_unique<ModelComponent>(isIce ? iceRockModel_ : rockModel_);
		if (isIce) {
			// 参照するマテリアルを氷に変更
			rock->SetBufferMaterial(0, iceMaterial_.GetMaterialSrvIndex());
			rock->SetHitGroup(1);
		}
		pool.push_back(std::move(rock));
	}
	return pool[activeNum++].get();
}
