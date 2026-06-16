#include "GamePhaseManager.h"

// 各フェーズ
#include "Phase/ScenePhase.h"

GamePhaseManager::GamePhaseManager(GameEngine::InputCommand* inputCommand, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager, CameraController* cameraController) {
	commonData_.inputCommand = inputCommand;
	commonData_.timeController_ = &timeController_;

	phases_[ScenePhase::kTitle] = std::make_unique<TitlePhase>(commonData_);
	phases_[ScenePhase::kPlay] = std::make_unique<PlayPhase>(commonData_, player, bossEnemy, playUIManager, cameraController);
	phases_[ScenePhase::kClear] = std::make_unique<ClearPhase>(commonData_);
}

void GamePhaseManager::Initialize() {

	commonData_.currentPhase = ScenePhase::kPlay;

	phases_[commonData_.currentPhase]->Enter();
}

void GamePhaseManager::Update() {

	// 時間の管理
	timeController_.Update();

	if (commonData_.requestPhase.has_value()) {
		phases_[commonData_.currentPhase]->Exit();
		commonData_.currentPhase = commonData_.requestPhase.value();
		commonData_.requestPhase = std::nullopt;
		phases_[commonData_.currentPhase]->Enter();
	}

	// 更新処理
	phases_[commonData_.currentPhase]->Update();
}
