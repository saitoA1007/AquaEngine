#include "HpBarUI.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;

HpBarUI::HpBarUI(std::string name) {

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>(name);
	debugParame_->Register("Base", baseWorld_);
	debugParame_->Register("BarSizeX", barSizeX_);
	debugParame_->Register("EffectmaxTime", effectmaxTime_);
	debugParame_->Register("Bar", barSprite_);
	debugParame_->Register("Effect", effectSprite_);
	debugParame_->Register("Frame", frameSprite_);

	// ペアレント
	barSprite_.SetParent(&baseWorld_);
	effectSprite_.SetParent(&baseWorld_);
	frameSprite_.SetParent(&baseWorld_);
}

void HpBarUI::Initialize() {
	debugParame_->Apply();
}

void HpBarUI::Update() {
	debugParame_->ApplyIfDirty();

	preScaleX_ = barSprite_.scale_.x;

	barSprite_.scale_.x = barSizeX_ * (static_cast<float>(currentHp_) / static_cast<float>(maxHp_));

	// 位置を設定する
	if (barSprite_.scale_.x != preScaleX_) {
		points_.push_back(Point(preScaleX_, barSprite_.scale_.x, 0.0f));
	}

	// 演出の更新処理
	EffectUpdate();

	barSprite_.Update();
	effectSprite_.Update();
	frameSprite_.Update();
}

void HpBarUI::Draw() {
	// 描画
	renderQueue_->SubmitSprite(&frameSprite_);
	renderQueue_->SubmitSprite(&effectSprite_);
	renderQueue_->SubmitSprite(&barSprite_);
}

void HpBarUI::EffectUpdate() {
	if (points_.size() != 0) {

		// 先頭要素を取得する
		auto& point = *points_.begin();

		point.timer += FpsCounter::deltaTime / effectmaxTime_;

		effectSprite_.scale_.x = Lerp(point.start, point.end, EaseIn(point.timer));

		if (point.timer >= 1.0f) {
			effectSprite_.scale_.x = point.end;

			// 削除
			points_.pop_front();
		}
	}
}