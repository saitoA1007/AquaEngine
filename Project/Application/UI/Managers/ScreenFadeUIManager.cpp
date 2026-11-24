#include "pch.h"
#include "ScreenFadeUIManager.h"
#include "TextureManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

ScreenFadeUIManager::ScreenFadeUIManager() {

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("ScreenFadeUI");
	int index = 0;
	debugParame_->Register("Color", color_, index++);
	debugParame_->Register("FadeInTime", fadeInTime_, index++);
	debugParame_->Register("FadeOutTime", fadeOutTime_, index++);
	debugParame_->Apply();

	// 画面全体を覆う白いスプライト
	sprite_.textureHandle_ = 0;
	sprite_.size_ = { 1280.0f,720.0f };
	sprite_.position_ = { 0.0f,0.0f };

	Initialize();
}

void ScreenFadeUIManager::Initialize() {
	phase_ = Phase::kNone;
	timer_ = 0.0f;
	ApplyAlpha(0.0f);
}

void ScreenFadeUIManager::Update() {
	debugParame_->ApplyIfDirty();

	switch (phase_) {
	case Phase::kNone:
		return;

	case Phase::kFadeIn:
		timer_ += FpsCounter::GetDeltaTime() / fadeInTime_;
		if (timer_ >= 1.0f) {
			timer_ = 0.0f;
			phase_ = Phase::kFull;
			ApplyAlpha(1.0f);
		} else {
			ApplyAlpha(Lerp(0.0f, 1.0f, timer_, EaseType::kEaseInQuad));
		}
		break;

	case Phase::kFull:
		ApplyAlpha(1.0f);
		break;

	case Phase::kFadeOut:
		timer_ += FpsCounter::GetDeltaTime() / fadeOutTime_;
		if (timer_ >= 1.0f) {
			Initialize();
			return;
		}
		ApplyAlpha(Lerp(1.0f, 0.0f, timer_, EaseType::kEaseOutQuad));
		break;
	}

	sprite_.Update();
}

void ScreenFadeUIManager::Draw() {
	if (phase_ == Phase::kNone) { return; }

	renderQueue_->SubmitSprite(&sprite_);
}

void ScreenFadeUIManager::StartFadeIn() {
	phase_ = Phase::kFadeIn;
	timer_ = 0.0f;
	ApplyAlpha(0.0f);
	sprite_.Update();
}

void ScreenFadeUIManager::StartFadeOut() {
	phase_ = Phase::kFadeOut;
	timer_ = 0.0f;
	ApplyAlpha(1.0f);
	sprite_.Update();
}

void ScreenFadeUIManager::ApplyAlpha(float alpha) {
	sprite_.color_ = color_;
	sprite_.color_.w = color_.w * alpha;
}
