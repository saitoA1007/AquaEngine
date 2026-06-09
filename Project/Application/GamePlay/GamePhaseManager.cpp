#include "GamePhaseManager.h"

// 各フェーズ
#include "Phase/ScenePhase.h"

GamePhaseManager::GamePhaseManager(Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager) {


	phases_[ScenePhase::kTitle] = std::make_unique<TitlePhase>(commonData_);
	phases_[ScenePhase::kPlay] = std::make_unique<PlayPhase>(commonData_, player,bossEnemy, playUIManager);
	phases_[ScenePhase::kClear] = std::make_unique<ClearPhase>(commonData_);
}

void GamePhaseManager::Initialize() {

	commonData_.currentPhase = ScenePhase::kPlay;

	phases_[commonData_.currentPhase]->Enter();
}

void GamePhaseManager::Update() {

	if (commonData_.requestPhase.has_value()) {
		phases_[commonData_.currentPhase]->Exit();
		commonData_.currentPhase = commonData_.requestPhase.value();
		commonData_.requestPhase = std::nullopt;
		phases_[commonData_.currentPhase]->Enter();
	}

	// 更新処理
	phases_[commonData_.currentPhase]->Update();
}
