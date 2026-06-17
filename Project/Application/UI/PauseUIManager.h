#pragma once
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Sprite.h"

class PauseUIManager : public GameEngine::IGameObject {
public:
	PauseUIManager();
	~PauseUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// ポーズ文字UI
	GameEngine::Sprite pauseTextSprite_;



};