#pragma once
#include "IGameObject.h"
#include "GameObjectManager.h"
#include "HpBarUI.h"

class PlayUIManager : public GameEngine::IGameObject {
public:
	PlayUIManager();
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

private:
	// ボスのHpUI
	std::unique_ptr<HpBarUI> bossHpBarUI_ = nullptr;
};