#include "PlayUIManager.h"

PlayUIManager::PlayUIManager() {

	// ボスUI
	bossHpBarUI_ = std::make_unique<HpBarUI>("BossHpUI");

	// プレイヤーUI
	playerHpUI_ = std::make_unique<HpContainer>("PlayerHpUI");
}

void PlayUIManager::Initialize() {
	bossHpBarUI_->Initialize();
	playerHpUI_->Initialize();
}

void PlayUIManager::Update() {
	bossHpBarUI_->Update();
	playerHpUI_->Update();
}

void PlayUIManager::Draw() {
	bossHpBarUI_->Draw();
	playerHpUI_->Draw();
}