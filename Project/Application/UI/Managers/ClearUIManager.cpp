#include "ClearUIManager.h"
#include "TextureManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

ClearUIManager::ClearUIManager(GameEngine::TextureManager* textureManager) {

	// クリア
	uint32_t clearTextGH = textureManager->GetHandleByName("titleText.png");

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("ClearUI");
	debugParame_->RegisterSprite("ButtonText", clearTextSprite_);
	debugParame_->Apply();

	// テクスチャを設定
	clearTextSprite_.textureHandle_ = clearTextGH;
}

void ClearUIManager::Initialize() {

}

void ClearUIManager::Update() {
	debugParame_->ApplyIfDirty();

	clearTextSprite_.Update();
}

void ClearUIManager::Draw() {

	renderQueue_->SubmitSprite(&clearTextSprite_);

}