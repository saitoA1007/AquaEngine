#include "pch.h"
#include "PauseUIManager.h"
#include "TextureManager.h"
#include "FontManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

namespace {
	// 選択項目の画面外の位置
	constexpr float kHideX = -256.0f;
	// 選択中の位置
	constexpr float kSelectX = 128.0f;
	// 非選択の位置
	constexpr float kNormalX = 96.0f;
}

PauseUIManager::PauseUIManager(GameEngine::TextureManager* textureManager, GameEngine::FontManager* fontManager) {

	// ポーズ
	uint32_t pauseTextGH = textureManager->GetHandleByName("pauseText.png");
	uint32_t backTextGH = textureManager->GetHandleByName("backText.png");
	uint32_t retryTextGH = textureManager->GetHandleByName("RetryText.png");
	uint32_t backTitleTextGH = textureManager->GetHandleByName("BackTitleText.png");
	uint32_t playGuideGH = textureManager->GetHandleByName("playGuide.png");

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("PauseUI");
	debugParame_->RegisterSprite("Bg", bgSprite_);
	debugParame_->RegisterSprite("Frame", frameSprite_);
	debugParame_->RegisterSprite("PlayGuide", playGuideSprite_);
	debugParame_->RegisterSprite("PauseText", pauseTextSprite_);

	// 戻る
	backUI_ = std::make_unique<DimmerUI>("BackUI", debugParame_.get(), true);
	// リトライ
	retryUI_ = std::make_unique<DimmerUI>("RetryUI", debugParame_.get(),true);
	// オプション
	optionUI_ = std::make_unique<DimmerUI>("OptionUI", debugParame_.get(), true);
	// タイトルへ
	backTitleUI_ = std::make_unique<DimmerUI>("BackTitleUI", debugParame_.get(),true);

	// パラメーターを適応
	debugParame_->Apply();

	// テクスチャを設定
	pauseTextSprite_.textureHandle_ = pauseTextGH;
	playGuideSprite_.textureHandle_ = playGuideGH;
	backUI_->SetTexture(backTextGH);
	retryUI_->SetTexture(retryTextGH);
	backTitleUI_->SetTexture(backTitleTextGH);
	// オプションは画像が無いので文字で表示
	optionUI_->SetText(fontManager->GetFont("Gisshiri"), "オプション", 44.0f);

	// SelectTypeの順番で並べる
	selectUIs_ = { backUI_.get(), retryUI_.get(), optionUI_.get(), backTitleUI_.get() };
}

void PauseUIManager::Initialize() {
	bgSprite_.color_.w = 0.0f;
	playGuideSprite_.color_.w = 0.0f;
	// 初期化位置
	frameSprite_.position_.x = -384.0f;
	pauseTextSprite_.position_.x = -384.0f;
	for (DimmerUI* ui : selectUIs_) {
		ui->world_.transform_.translate.x = kHideX;
	}
}

void PauseUIManager::Update() {
	debugParame_->ApplyIfDirty();

	// アニメーション
	if(isAnimation_) {

		Animation();
	} else {
		// 選択中の項目だけ前に出す
		for (size_t i = 0; i < selectUIs_.size(); ++i) {
			bool isSelect = (static_cast<SelectType>(i) == type_);
			selectUIs_[i]->world_.transform_.translate.x = isSelect ? kSelectX : kNormalX;
		}
	}

	bgSprite_.Update();
	frameSprite_.Update();
	playGuideSprite_.Update();
	for (DimmerUI* ui : selectUIs_) {
		ui->Update();
	}
	pauseTextSprite_.Update();
}

void PauseUIManager::Draw() {

	renderQueue_->SubmitSprite(&bgSprite_);
	renderQueue_->SubmitSprite(&frameSprite_);
	renderQueue_->SubmitSprite(&playGuideSprite_);
	renderQueue_->SubmitSprite(&pauseTextSprite_);
	for (DimmerUI* ui : selectUIs_) {
		ui->Draw();
	}
}

void PauseUIManager::Animation() {

	switch (phase_)
	{
	case PauseUIManager::Phase::kIn:
		timer_ += FpsCounter::GetDeltaTime() / kInMaxTime_;

		// 背景の表示
		if (timer_ <= 0.5f) {
			float localT = timer_ / 0.5f;
			bgSprite_.color_.w = Lerp(0.0f, 0.5f, localT);
		}

		// フレームの移動
		if (timer_ <= 0.8f) {
			float localT = timer_ / 0.8f;
			frameSprite_.position_.x = Lerp(-384.0f, 0.0f, localT, EaseType::kEaseOutQuad);
		}
		// ポーズ文字の移動
		if (timer_ <= 0.9f) {
			float localT = timer_ / 0.9f;
			pauseTextSprite_.position_.x = Lerp(-384.0f, 192.0f, localT, EaseType::kEaseOutQuad);
		}

		// 選択テキストの移動
		for (size_t i = 0; i < selectUIs_.size(); ++i) {
			float targetX = (static_cast<SelectType>(i) == type_) ? kSelectX : kNormalX;
			selectUIs_[i]->world_.transform_.translate.x = Lerp(kHideX, targetX, timer_, EaseType::kEaseInQuad);
		}

		// 操作説明の表示
		playGuideSprite_.color_.w = Lerp(0.0f, 1.0f, timer_, EaseType::kEaseInQuad);

		if (timer_ >= 1.0f) {
			frameSprite_.position_.x = 0.0f;
			pauseTextSprite_.position_.x = 192.0f;
			for (size_t i = 0; i < selectUIs_.size(); ++i) {
				selectUIs_[i]->world_.transform_.translate.x = (static_cast<SelectType>(i) == type_) ? kSelectX : kNormalX;
			}
			playGuideSprite_.color_.w = 1.0f;
			isAnimation_ = false;
		}
		break;

	case PauseUIManager::Phase::kOut:
		timer_ += FpsCounter::GetDeltaTime() / kOutMaxTime_;

		// 背景の表示
		bgSprite_.color_.w = Lerp(0.5f, 0.0f, timer_);

		// フレームの移動
		frameSprite_.position_.x = Lerp(0.0f, -384.0f, timer_, EaseType::kEaseInQuad);

		// ポーズ文字の移動
		if (timer_ <= 0.9f) {
			float localT = timer_ / 0.9f;
			pauseTextSprite_.position_.x = Lerp(192.0f, -384.0f, localT, EaseType::kEaseOutQuad);
		}
		// 選択テキストの移動
		if (timer_ <= 0.8f) {
			float localT = timer_ / 0.8f;
			// 選択テキストの移動
			for (DimmerUI* ui : selectUIs_) {
				ui->world_.transform_.translate.x = Lerp(kNormalX, kHideX, localT, EaseType::kEaseOutQuad);
			}

			// 操作説明の表示
			playGuideSprite_.color_.w = Lerp(1.0f, 0.0f, localT, EaseType::kEaseOutQuad);
		}

		if (timer_ >= 1.0f) {
			playGuideSprite_.color_.w = 0.0f;
			for (DimmerUI* ui : selectUIs_) {
				ui->world_.transform_.translate.x = kHideX;
			}
			frameSprite_.position_.x = -384.0f;
			isAnimation_ = false;
		}
		break;
	}
}
