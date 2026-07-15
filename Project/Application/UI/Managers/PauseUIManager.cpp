#include "PauseUIManager.h"

using namespace GameEngine;

PauseUIManager::PauseUIManager() {

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("PauseUI");

}

void PauseUIManager::Initialize() {

}

void PauseUIManager::Update() {
	debugParame_->ApplyIfDirty();
}

void PauseUIManager::Draw() {



}