#pragma once
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Sprite.h"

// 画面全体を単色のスプライトで覆って遷移させる
class ScreenFadeUIManager : public GameEngine::IGameObject {
public:
	// フェードの段階
	enum class Phase {
		kNone,    // 何もしていない
		kFadeIn,  // 画面を覆っていく
		kFull,    // 画面を覆ったまま
		kFadeOut, // 画面を元に戻していく
	};

public:
	ScreenFadeUIManager();
	~ScreenFadeUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 画面を覆い始める
	void StartFadeIn();

	// 画面を元に戻し始める
	void StartFadeOut();

	// 画面を完全に覆っているか
	bool IsFull() const { return phase_ == Phase::kFull; }

	// フェード中か
	bool IsFading() const { return phase_ == Phase::kFadeIn || phase_ == Phase::kFadeOut; }

	// 現在の段階を取得
	Phase GetPhase() const { return phase_; }

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 画面を覆うスプライト
	GameEngine::Sprite sprite_;

	// 色
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };

	// 覆うまでの時間
	float fadeInTime_ = 0.6f;
	// 元に戻すまでの時間
	float fadeOutTime_ = 0.6f;

	Phase phase_ = Phase::kNone;
	float timer_ = 0.0f;

private:
	// 透明度を設定する
	void ApplyAlpha(float alpha);
};
