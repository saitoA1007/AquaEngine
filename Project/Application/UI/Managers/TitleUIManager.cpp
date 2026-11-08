#include "pch.h"
#include "TitleUIManager.h"
#include "TextureManager.h"
#include "FontManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

TitleUIManager::TitleUIManager(GameEngine::TextureManager* textureManager, GameEngine::FontManager* fontManager) {

	// オプションの操作説明
	optionGuideText_ = std::make_unique<Text>(fontManager->GetFont("Gisshiri"), "Viewボタン：オプション", 24);
	optionGuideText_->anchorPoint_ = { 0.5f,0.5f };
	optionGuideText_->position_ = { 640.0f,680.0f };
	optionGuideText_->scale_ = { 24.0f,24.0f };

	// テクスチャを取得
	uint32_t titleTextGH = textureManager->GetHandleByName("titleText.png");
	uint32_t buttonTextGH = textureManager->GetHandleByName("spaceText.png");

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("TitleUI");
	debugParame_->RegisterSprite("TitleText", titleTextSprite_);
	debugParame_->RegisterSprite("ButtonText", buttonTextSprite_);
	debugParame_->Register("OptionGuidePos", optionGuideText_->position_, 0, "OptionGuide");
	debugParame_->Register("OptionGuideScale", optionGuideText_->scale_, 0, "OptionGuide");
	debugParame_->Register("OptionGuideColor", optionGuideText_->color_, 0, "OptionGuide");
	debugParame_->Apply();

	// テクスチャを設定
	titleTextSprite_.textureHandle_ = titleTextGH;
	buttonTextSprite_.textureHandle_ = buttonTextGH;
}

void TitleUIManager::Initialize() {
	titleTextSprite_.color_.w = 1.0f;
	buttonTextSprite_.color_.w = 1.0f;
	optionGuideText_->color_.w = 1.0f;
	timer_ = 0.0f;
	isDraw_ = true;
	isActiveFadeOut_ = false;
}

void TitleUIManager::Update() {
	debugParame_->ApplyIfDirty();

	if (!isDraw_ && isActiveFadeOut_) {
		timer_ += FpsCounter::GetGameDeltaTime() / kFadeOutMaxTime_;

		// 透明にする
		float alpha = Lerp(1.0f, 0.0f, timer_);
		titleTextSprite_.color_.w = alpha;
		buttonTextSprite_.color_.w = alpha;
		optionGuideText_->color_.w = alpha;

		if (timer_ >= 1.0f) {
			titleTextSprite_.color_.w = 0.0f;
			buttonTextSprite_.color_.w = 0.0f;
			optionGuideText_->color_.w = 0.0f;
			isActiveFadeOut_ = false;
		}
	}

	// 更新処理
	titleTextSprite_.Update();
	buttonTextSprite_.Update();
	optionGuideText_->Update();
}

void TitleUIManager::Draw() {
	if (isDraw_ || isActiveFadeOut_) {
		renderQueue_->SubmitSprite(&titleTextSprite_);
		renderQueue_->SubmitSprite(&buttonTextSprite_);
		renderQueue_->SubmitText(optionGuideText_.get());
	}	
}