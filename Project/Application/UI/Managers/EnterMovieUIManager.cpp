#include "pch.h"
#include "EnterMovieUIManager.h"
#include "FontManager.h"
#include "TextureManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

EnterMovieUIManager::EnterMovieUIManager(GameEngine::FontManager* fontManager, GameEngine::TextureManager* textureManager) :
	downtext_(fontManager->GetFont("Gisshiri"), "ジークアイス") {

	// 上側は画像で表示する(画像は1280x256)
	upSprite_.textureHandle_ = textureManager->GetHandleByName("bossUpText.png");
	upSprite_.size_ = { 1280.0f,256.0f };
	upSprite_.scale_ = { 0.5f,0.5f };

	// 中心を基準にする
	upSprite_.anchorPoint_ = { 0.5f,0.5f };
	downtext_.anchorPoint_ = { 0.5f,0.5f };
	downtext_.scale_ = { 48.0f,48.0f };

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("EnterMovieUI");
	std::string subGroup = "UpImage";
	int index = 0;
	debugParame_->Register("TargetPos", upTargetPos_, index++, subGroup);
	debugParame_->Register("Size", upSprite_.size_, index++, subGroup);
	debugParame_->Register("Scale", upSprite_.scale_, index++, subGroup);
	debugParame_->Register("Color", upColor_, index++, subGroup);
	subGroup = "DownText";
	index = 0;
	debugParame_->Register("TargetPos", downTargetPos_, index++, subGroup);
	debugParame_->Register("Scale", downtext_.scale_, index++, subGroup);
	debugParame_->Register("Color", downColor_, index++, subGroup);
	subGroup = "Animation";
	index = 0;
	debugParame_->Register("MoveOffsetY", moveOffsetY_, index++, subGroup);
	debugParame_->Register("InTime", inTime_, index++, subGroup);
	debugParame_->Register("HoldTime", holdTime_, index++, subGroup);
	debugParame_->Register("OutTime", outTime_, index++, subGroup);
	debugParame_->Apply();

	Initialize();
}

void EnterMovieUIManager::Initialize() {
	// 表示しない状態に戻す
	Stop();
}

void EnterMovieUIManager::Update() {
	debugParame_->ApplyIfDirty();

	switch (phase_) {
	case Phase::kNone:
		return;

	case Phase::kIn:
		timer_ += FpsCounter::deltaTime / inTime_;
		if (timer_ >= 1.0f) {
			timer_ = 0.0f;
			phase_ = Phase::kHold;
			ApplyAnimation(1.0f, 1.0f);
		} else {
			// 上下から表示する位置へ動かしながら表示する
			ApplyAnimation(timer_, timer_);
		}
		break;

	case Phase::kHold:
		timer_ += FpsCounter::deltaTime / holdTime_;
		ApplyAnimation(1.0f, 1.0f);
		if (timer_ >= 1.0f) {
			timer_ = 0.0f;
			phase_ = Phase::kOut;
		}
		break;

	case Phase::kOut:
		timer_ += FpsCounter::deltaTime / outTime_;
		if (timer_ >= 1.0f) {
			Stop();
			return;
		}
		// その場で消える
		ApplyAnimation(1.0f, 1.0f - timer_);
		break;
	}

	upSprite_.Update();
	downtext_.Update();
}

void EnterMovieUIManager::Draw() {
	if (phase_ == Phase::kNone) { return; }

	renderQueue_->SubmitSprite(&upSprite_);
	renderQueue_->SubmitText(&downtext_);
}

void EnterMovieUIManager::Play() {
	phase_ = Phase::kIn;
	timer_ = 0.0f;
	ApplyAnimation(0.0f, 0.0f);
	upSprite_.Update();
	downtext_.Update();
}

void EnterMovieUIManager::Stop() {
	phase_ = Phase::kNone;
	timer_ = 0.0f;
	ApplyAnimation(0.0f, 0.0f);
}

void EnterMovieUIManager::ApplyAnimation(float moveT, float alpha) {
	// 上の文字は上側から、下の文字は下側から入ってくる
	upSprite_.position_.x = upTargetPos_.x;
	upSprite_.position_.y = Lerp(upTargetPos_.y - moveOffsetY_, upTargetPos_.y, moveT, EaseType::kEaseOutCubic);
	downtext_.position_.x = downTargetPos_.x;
	downtext_.position_.y = Lerp(downTargetPos_.y + moveOffsetY_, downTargetPos_.y, moveT, EaseType::kEaseOutCubic);

	upSprite_.color_ = upColor_;
	upSprite_.color_.w = upColor_.w * alpha;
	downtext_.color_ = downColor_;
	downtext_.color_.w = downColor_.w * alpha;
}
