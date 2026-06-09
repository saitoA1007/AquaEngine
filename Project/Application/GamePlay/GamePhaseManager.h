#pragma once
#include <unordered_map>
#include <memory>
#include "IGameObject.h"
#include "IScenePhase.h"

// 前方宣言
class Player;
class PlayUIManager;
class BossEnemy;

class GamePhaseManager : public GameEngine::IGameObject {
public:
	GamePhaseManager(Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager);
	~GamePhaseManager() = default;

	void Initialize() override;

	// 更新処理
	void Update() override;

private:

	// 各フェーズ
	std::unordered_map<ScenePhase, std::unique_ptr<IScenePhase>> phases_;

	// 共通データ
	PhaseCommonData commonData_;
};