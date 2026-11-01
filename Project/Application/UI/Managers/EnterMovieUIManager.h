#pragma once
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Text.h"
#include "Sprite.h"

// 前方宣言
namespace GameEngine {
	class FontManager;
	class TextureManager;
}

class EnterMovieUIManager : public GameEngine::IGameObject {
public:
	// アニメーションの段階
	enum class Phase {
		kNone, // 表示しない
		kIn,   // 上下から入ってくる
		kHold, // 表示したまま
		kOut,  // 消える
	};

public:
	EnterMovieUIManager(GameEngine::FontManager* fontManager, GameEngine::TextureManager* textureManager);
	~EnterMovieUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// ボスの名前の表示を開始する
	void Play();

	// 表示を止める
	void Stop();

	// 表示中か
	bool IsPlay() const { return phase_ != Phase::kNone; }

private:

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 上側に表示する画像
	GameEngine::Sprite upSprite_;
	// 下側に表示するテキスト
	GameEngine::Text downtext_;

	// 表示する位置
	Vector2 upTargetPos_ = { 640.0f,200.0f };
	Vector2 downTargetPos_ = { 640.0f,520.0f };
	// 表示する位置から、動き始める位置までのずれ
	float moveOffsetY_ = 120.0f;

	// 文字の色
	Vector4 upColor_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector4 downColor_ = { 1.0f,1.0f,1.0f,1.0f };

	// 入ってくる時間
	float inTime_ = 0.4f;
	// 表示したままの時間
	float holdTime_ = 1.2f;
	// 消える時間
	float outTime_ = 0.3f;

	// 現在の段階
	Phase phase_ = Phase::kNone;
	float timer_ = 0.0f;

private:

	// 文字の位置と透明度を設定する
	void ApplyAnimation(float moveT, float alpha);
};
