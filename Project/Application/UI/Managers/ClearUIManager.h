#pragma once
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Sprite.h"

// 前方宣言
namespace GameEngine {
	class TextureManager;
}

/// <summary>
/// クリアUIの管理
/// </summary>
class ClearUIManager : public GameEngine::IGameObject {
public:
	ClearUIManager(GameEngine::TextureManager* textureManager);
	~ClearUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// クリア文字UI
	GameEngine::Sprite clearTextSprite_;



};