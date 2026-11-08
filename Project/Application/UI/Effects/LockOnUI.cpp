#include "pch.h"
#include "LockOnUI.h"
#include <numbers>
#include "EasingManager.h"
#include "FPSCounter.h"
#include "MyMath.h"
using namespace GameEngine;

LockOnUI::LockOnUI(std::string name, uint32_t lockOnUIGH) {
	name_ = name;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>(name_);
	int i = 0;
	debugParame_->Register("ArrowSize", arrowSize_, i++);
	debugParame_->Register("Color", color_, i++);
	debugParame_->Register("TargetOffsetY", targetOffsetY_, i++);
	debugParame_->Register("MinRadius", minRadius_, i++);
	debugParame_->Register("MaxRadius", maxRadius_, i++);
	debugParame_->Register("MinScale", minScale_, i++);
	debugParame_->Register("MaxScale", maxScale_, i++);
	debugParame_->Register("LoopTime", loopTime_, i++);
	debugParame_->Register("EnterTime", enterTime_, i++);
	debugParame_->Register("EnterRadiusRate", enterRadiusRate_, i++);
	debugParame_->Register("StartAngleDeg", startAngleDeg_, i++);
	debugParame_->Apply();

	for (auto& sprite : arrowSprites_) {
		sprite.textureHandle_ = lockOnUIGH;
		sprite.anchorPoint_ = { 0.5f,0.5f };
		sprite.size_ = arrowSize_;
		sprite.color_ = { 1.0f,1.0f,1.0f,1.0f };
		sprite.SetParent(&world_);
	}
}

void LockOnUI::Initialize() {
	isLockOn_ = false;
	isVisible_ = false;
	target_ = nullptr;
	enterTimer_ = 1.0f;
	loopTimer_ = 0.0f;
}

void LockOnUI::SetLockOn(bool isLockOn, const GameEngine::WorldTransform* target) {
	// ロックオンが開始された時に出現アニメーションを始める
	if (isLockOn && !isLockOn_) {
		enterTimer_ = 0.0f;
		loopTimer_ = 0.0f;
	}
	isLockOn_ = isLockOn;
	target_ = target;
}

void LockOnUI::Update() {
	debugParame_->ApplyIfDirty();

	isVisible_ = false;
	if (!isLockOn_ || !target_) { return; }

	// 対象のワールド座標をスクリーン座標に変換
	Vector3 targetPos = target_->GetWorldPosition();
	targetPos.y += targetOffsetY_;
	const Camera& camera = renderQueue_->GetMainCamera();
	// カメラの後ろにいる場合は表示しない
	if (Math::IsBehindCamera(targetPos, camera.GetViewMatrix())) { return; }
	const Vector2 screenPos = Math::WorldToScreen(targetPos, camera.GetVPMatrix());
	isVisible_ = true;

	// アニメーション
	UpdateAnimation();

	// 矢印の配置
	UpdateArrowLayout(screenPos);
}

void LockOnUI::UpdateAnimation() {

	const float deltaTime = FpsCounter::GetGameDeltaTime();

	// 出現
	float radiusRate = 1.0f;
	animAlpha_ = 1.0f;
	if (enterTimer_ < 1.0f) {
		enterTimer_ = (enterTime_ > 0.0f) ? std::min(enterTimer_ + deltaTime / enterTime_, 1.0f) : 1.0f;
		radiusRate = Lerp(enterRadiusRate_, 1.0f, enterTimer_, EaseType::kEaseOutBack);
		animAlpha_ = Lerp(0.0f, 1.0f, enterTimer_, EaseType::kEaseOutQuad);
	}

	// 拡縮
	if (loopTime_ > 0.0f) {
		loopTimer_ += deltaTime / loopTime_;
		loopTimer_ -= std::floor(loopTimer_);
	}
	float wave = 0.5f - 0.5f * std::cosf(loopTimer_ * 2.0f * std::numbers::pi_v<float>);
	animRadius_ = Lerp(maxRadius_, minRadius_, wave) * radiusRate;
	animScale_ = Lerp(minScale_, maxScale_, wave);
}

void LockOnUI::UpdateArrowLayout(const Vector2& screenPos) {

	// 中心をロックオン対象の画面位置に合わせる
	world_.transform_.translate = { screenPos.x, screenPos.y, 0.0f };
	world_.UpdateTransformMatrix();

	const float startAngle = startAngleDeg_ * (std::numbers::pi_v<float> / 180.0f);
	for (uint32_t i = 0; i < kArrowNum; ++i) {
		Sprite& sprite = arrowSprites_[i];
		float angle = startAngle + (2.0f * std::numbers::pi_v<float> / static_cast<float>(kArrowNum)) * static_cast<float>(i);

		// 中心から円状に配置
		sprite.position_ = { std::cosf(angle) * animRadius_, std::sinf(angle) * animRadius_ };
		// 画像は下向きの矢印なので、中心を向くように回転させる
		sprite.rotate_ = angle + std::numbers::pi_v<float> * 0.5f;
		sprite.scale_ = { animScale_, animScale_ };
		sprite.size_ = arrowSize_;
		sprite.color_ = { color_.x, color_.y, color_.z, color_.w * animAlpha_ };
		sprite.Update();
	}
}

void LockOnUI::Draw() {
	if (!isLockOn_ || !isVisible_) { return; }

	// 描画
	for (auto& sprite : arrowSprites_) {
		renderQueue_->SubmitSprite(&sprite);
	}
}
