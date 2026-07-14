#include "PlayUIManager.h"

using namespace GameEngine;

PlayUIManager::PlayUIManager(uint32_t playerHpGH, uint32_t bossNameGH, uint32_t playGuideGH) {

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("PlayUI");
	debugParame_->RegisterSprite("BossName", bossNameSprite_);
	debugParame_->RegisterSprite("PlayGuide", playGuideSprite_);
	debugParame_->Apply();

	// テクスチャを設定
	bossNameSprite_.textureHandle_ = bossNameGH;
	playGuideSprite_.textureHandle_ = playGuideGH;

	// ボスUI
	bossHpBarUI_ = std::make_unique<HpBarUI>("BossHpUI");

	// プレイヤーUI
	playerHpUI_ = std::make_unique<HpContainer>("PlayerHpUI", playerHpGH);

	// 黒帯UI
	letterBoxUI_ = std::make_unique<LetterboxUI>("LetterboxUI");
}

void PlayUIManager::Initialize() {
	bossHpBarUI_->Initialize();
	playerHpUI_->Initialize();
	letterBoxUI_->Initialize();
}

void PlayUIManager::Update() {
	debugParame_->ApplyIfDirty();

	bossNameSprite_.Update();
	playGuideSprite_.Update();
	bossHpBarUI_->Update();
	playerHpUI_->Update();
	letterBoxUI_->Update();
}

void PlayUIManager::Draw() {
	// 黒帯を描画
	letterBoxUI_->Draw();

	if (isDrawGamePlayUI_) {
		// ボスHpを描画
		bossHpBarUI_->Draw();
		renderQueue_->SubmitSprite(&bossNameSprite_);
		// プレイヤーHpを描画
		playerHpUI_->Draw();
	}
	
	if (isDrawPlayGuide_) {
		renderQueue_->SubmitSprite(&playGuideSprite_);
	}
}