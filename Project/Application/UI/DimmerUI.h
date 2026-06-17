#pragma once
#include "IGameObject.h"
#include "Sprite.h"
#include "DebugParameter.h"

class DimmerUI : public GameEngine::IGameObject {
public:
	DimmerUI(std::string name, GameEngine::DebugParameter* debugParame);
	~DimmerUI() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 基準点に親を設定
	void SetParent(GameEngine::WorldTransform* parent) {
		sprite_.SetParent(parent);
	}

private:

	// 時間
	float maxTime_ = 1.0f;

	// 動く倍率
	float scaleRatio_ = 0.75f;

private:

	// 画像
	GameEngine::Sprite sprite_;

	bool isPlay_ = false;

	float timer_ = 0.0f;

	Vector2 startScale_;
	Vector2 endScale_;

	std::string name_;
};