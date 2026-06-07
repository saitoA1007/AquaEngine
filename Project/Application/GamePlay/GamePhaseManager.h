#pragma once
#include <unordered_map>
#include <memory>
#include "IGameObject.h"
#include "IScenePhase.h"

class GamePhaseManager : public GameEngine::IGameObject {
public:
	GamePhaseManager();
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