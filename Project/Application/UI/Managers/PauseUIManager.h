#pragma once
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Sprite.h"

// 前方宣言
namespace GameEngine {
	class TextureManager;
}

/// <summary>
/// ポーズUIの管理
/// </summary>
class PauseUIManager : public GameEngine::IGameObject {
public:
	PauseUIManager(GameEngine::TextureManager* textureManager);
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

	// 半透明の背景
	GameEngine::Sprite bgSprite_;
	// 隠れるフレーム部分
	GameEngine::Sprite frameSprite_;

};