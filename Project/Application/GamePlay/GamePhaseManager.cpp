#include "GamePhaseManager.h"

// 各フェーズ
#include "Phase/ScenePhase.h"

GamePhaseManager::GamePhaseManager(GameEngine::InputCommand* inputCommand, Player* player, BossEnemy* bossEnemy,
	TitleUIManager* titleUIManager, PlayUIManager* playUIManager, CameraController* cameraController) {
	commonData_.inputCommand = inputCommand;
	commonData_.timeController_ = &timeController_;

	phases_[ScenePhase::kTitle] = std::make_unique<TitlePhase>(commonData_, titleUIManager);
	phases_[ScenePhase::kTutorial] = std::make_unique<TutorialPhase>(commonData_, cameraController, bossEnemy, playUIManager);
	phases_[ScenePhase::kPlay] = std::make_unique<PlayPhase>(commonData_, player, bossEnemy, playUIManager, cameraController);
	phases_[ScenePhase::kGameOver] = std::make_unique<GameOverPhase>(commonData_);
	phases_[ScenePhase::kClear] = std::make_unique<ClearPhase>(commonData_);
	phases_[ScenePhase::kPause] = std::make_unique<PausePhase>(commonData_);
}

void GamePhaseManager::Initialize() {

	commonData_.currentPhase = ScenePhase::kTitle;

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
