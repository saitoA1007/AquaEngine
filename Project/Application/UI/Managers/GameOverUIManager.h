#pragma once
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Sprite.h"

// 前方宣言
namespace GameEngine {
	class TextureManager;
}

/// <summary>
/// ゲームオーバーUIの管理
/// </summary>
class GameOverUIManager : public GameEngine::IGameObject {
public:
	GameOverUIManager(GameEngine::TextureManager* textureManager);
	~GameOverUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// ゲームオーバー文字UI
	GameEngine::Sprite gameOverTextSprite_;



};