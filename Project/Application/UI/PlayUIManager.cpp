#include "PlayUIManager.h"

PlayUIManager::PlayUIManager() {

	// ボスUI
	bossHpBarUI_ = std::make_unique<HpBarUI>("BossHpUI");

}

void PlayUIManager::Initialize() {
	bossHpBarUI_->Initialize();
}

void PlayUIManager::Update() {
	bossHpBarUI_->Update();
}

void PlayUIManager::Draw() {
	bossHpBarUI_->Draw();
}