#pragma once
#include <list>
#include "IGameObject.h"
#include "Model.h"

class StageManager : public GameEngine::IGameObject {
public:
	StageManager(GameEngine::Model* model);
	~StageManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	GameEngine::Model* model_ = nullptr;


};