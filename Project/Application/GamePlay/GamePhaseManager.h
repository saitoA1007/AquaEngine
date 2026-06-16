#pragma once
#include <unordered_map>
#include <memory>
#include "IGameObject.h"
#include "IScenePhase.h"
#include "Application/Utils/TimeController.h"

// 前方宣言
namespace GameEngine {
	class InputCommand;
}
class Player;
class PlayUIManager;
class BossEnemy;
class CameraController;

class GamePhaseManager : public GameEngine::IGameObject {
public:
	GamePhaseManager(GameEngine::InputCommand* inputCommand, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager, CameraController* cameraController);
	~GamePhaseManager() = default;

	void Initialize() override;

	// 更新処理
	void Update() override;

private:

	// 各フェーズ
	std::unordered_map<ScenePhase, std::unique_ptr<IScenePhase>> phases_;

	// 共通データ
	PhaseCommonData commonData_;

	// 時間の管理
	TimeController timeController_;
};