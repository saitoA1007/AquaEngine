#pragma once
#include <list>
#include "IGameObject.h"
#include "Sprite.h"
#include "WorldTransform.h"

class HpBarUI : public GameEngine::IGameObject {
public:

	struct Point {
		float start;
		float end;
		float timer = 0.0f;
	};

public:
	HpBarUI();
	~HpBarUI() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:

	// 基準の位置
	GameEngine::WorldTransform baseWorld_;

	// 体力
	GameEngine::Sprite barSprite_;

	// hpの減少演出用
	GameEngine::Sprite effectSprite_;

	// フレーム
	GameEngine::Sprite frameSprite_;

	// 最大hp
	int32_t maxHp_ = 0;

	// 現在のhp
	int32_t currentHp_ = 0;

	// 演出用のhpゲージが移動する位置
	std::list<Point> points_;

	float maxTime_ = 0.5f;
};