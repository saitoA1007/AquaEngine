#pragma once
#include <array>
#include "IGameObject.h"
#include "Sprite.h"
#include "DebugParameter.h"

/// <summary>
/// ロックオン対象の周りに4つの矢印を円状に並べて表示するUI
/// </summary>
class LockOnUI : public GameEngine::IGameObject {
public:
	LockOnUI(std::string name,uint32_t lockOnUIGH);
	~LockOnUI() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	/// <summary>
	/// ロックオン状態と対象を設定
	/// </summary>
	/// <param name="isLockOn">ロックオン中か</param>
	/// <param name="target">ロックオン対象</param>
	void SetLockOn(bool isLockOn, const GameEngine::WorldTransform* target);

public:
	// 基準
	GameEngine::WorldTransform world_;

private: // 調整項目

	// 矢印の大きさ
	Vector2 arrowSize_ = { 32.0f,32.0f };

	// 矢印の色
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };

	// 対象からのY方向のオフセット
	float targetOffsetY_ = 0.0f;

	// 中心から矢印までの距離
	float minRadius_ = 40.0f;
	float maxRadius_ = 56.0f;

	// 矢印のスケール
	float minScale_ = 0.8f;
	float maxScale_ = 1.1f;

	// 拡縮の1周期の時間
	float loopTime_ = 0.8f;

	// 出現アニメーションの時間
	float enterTime_ = 0.25f;
	// 出現開始時の距離の倍率
	float enterRadiusRate_ = 3.0f;

	// 円状に並べる時の開始角度
	float startAngleDeg_ = 45.0f;

private:
	// 矢印の数
	static constexpr uint32_t kArrowNum = 4;

	// 名前
	std::string name_;

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 矢印の画像
	std::array<GameEngine::Sprite, kArrowNum> arrowSprites_;

	// ロックオン対象
	const GameEngine::WorldTransform* target_ = nullptr;

	// ロックオン中か
	bool isLockOn_ = false;
	// 対象が画面に映っているか
	bool isVisible_ = false;

	// 出現アニメーションのタイマー
	float enterTimer_ = 1.0f;
	// 拡縮ループのタイマー
	float loopTimer_ = 0.0f;

	// アニメーション結果
	float animRadius_ = 0.0f;
	float animScale_ = 1.0f;
	float animAlpha_ = 1.0f;

private:

	// アニメーションを更新
	void UpdateAnimation();

	// アニメーション結果を元に矢印を円状に配置
	void UpdateArrowLayout(const Vector2& screenPos);
};
