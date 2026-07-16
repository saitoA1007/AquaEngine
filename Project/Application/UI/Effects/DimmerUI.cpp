#include "DimmerUI.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

DimmerUI::DimmerUI(std::string name, GameEngine::DebugParameter* debugParame) {
	name_ = name;

	int i = 0;
	debugParame->Register("MaxTime", maxTime_, i++, name_);
	debugParame->Register("ScaleRatio", scaleRatio_, i++, name_);
	debugParame->RegisterSprite("Sprite", sprite_, name_);

}

void DimmerUI::Initialize() {
	startScale_ = sprite_.scale_;
	endScale_ = startScale_ * scaleRatio_;
	timer_ = 0.0f;
	isPlay_ = false;
}

void DimmerUI::Update() {

	if (isPlay_) {
		timer_ += FpsCounter::gameDeltaTime / maxTime_;

		if (timer_ <= 0.5f) {
			float localT = timer_ / 0.5f;
			sprite_.scale_ = Lerp(startScale_, endScale_, EaseInOut(localT));
		} else {
			float localT = (timer_ - 0.5f) / 0.5f;
			sprite_.scale_ = Lerp(endScale_, startScale_, EaseInOut(localT));
		}

		if (timer_ >= 1.0f) {
			sprite_.scale_ = startScale_;
			timer_ = 0.0f;
			isPlay_ = false;
		}
	}

	sprite_.Update();
}

void DimmerUI::Draw() {
	// 描画
	renderQueue_->SubmitSprite(&sprite_);
}