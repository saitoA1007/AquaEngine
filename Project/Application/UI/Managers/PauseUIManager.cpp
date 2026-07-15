#include "PauseUIManager.h"
#include "TextureManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

PauseUIManager::PauseUIManager(GameEngine::TextureManager* textureManager) {

	// ポーズ
	uint32_t gameOverTextGH = textureManager->GetHandleByName("titleText.png");

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("PauseUI");
	debugParame_->RegisterSprite("ButtonText", pauseTextSprite_);
	debugParame_->Apply();

	// テクスチャを設定
	pauseTextSprite_.textureHandle_ = gameOverTextGH;

}

void PauseUIManager::Initialize() {

}

void PauseUIManager::Update() {
	debugParame_->ApplyIfDirty();

	pauseTextSprite_.Update();
}

void PauseUIManager::Draw() {


	renderQueue_->SubmitSprite(&pauseTextSprite_);
}