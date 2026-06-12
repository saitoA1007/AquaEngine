#pragma once
#include "IGameObject.h"
#include "HpBarUI.h"
#include "HpContainer.h"

class PlayUIManager : public GameEngine::IGameObject {
public:
	PlayUIManager(uint32_t playerHpGH, uint32_t bossNameGH, uint32_t playGuideGH);
	~PlayUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	void SetCurrentBossHp(int32_t hp) {
		bossHpBarUI_->SetCurrentHp(hp);
	}

	void SetMaxBossHp(int32_t hp) {
		bossHpBarUI_->SetMaxHp(hp);
	}

	void SetCurrentPlayerHp(int32_t hp) {
		playerHpUI_->SetCurrentHp(hp);
	}

	void SetMaxPlayerHp(int32_t hp) {
		playerHpUI_->SetMaxHp(hp);
	}

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// ボスのHpUI
	std::unique_ptr<HpBarUI> bossHpBarUI_;

	// ボスの名前
	GameEngine::Sprite bossNameSprite_;

	// プレイヤーHpUI
	std::unique_ptr<HpContainer> playerHpUI_;

	// 操作説明UI
	GameEngine::Sprite playGuideSprite_;
};