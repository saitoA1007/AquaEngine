#include "GameOverUIManager.h"
#include "TextureManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

GameOverUIManager::GameOverUIManager(GameEngine::TextureManager* textureManager) {

	// ゲームオーバー
	uint32_t gameOverTextGH = textureManager->GetHandleByName("titleText.png");

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("GameOverUI");
	debugParame_->RegisterSprite("ButtonText", gameOverTextSprite_);
	debugParame_->Apply();

	// テクスチャを設定
	gameOverTextSprite_.textureHandle_ = gameOverTextGH;
}

void GameOverUIManager::Initialize() {

}

void GameOverUIManager::Update() {
	debugParame_->ApplyIfDirty();

	gameOverTextSprite_.Update();
}

void GameOverUIManager::Draw() {


	renderQueue_->SubmitSprite(&gameOverTextSprite_);
}