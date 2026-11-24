#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "Collider.h"
#include "EaseCurve.h"

// 氷柱の中に入っている回復用のハート
class HealingHeart : public GameEngine::IGameObject {
public:
	// ハートの状態
	enum class State {
		kInIce,    // 氷柱の中に入っている
		kReleased, // 氷柱が壊れて取れる状態
	};

public:
	HealingHeart(GameEngine::Model* model);
	~HealingHeart() = default;

	// 初期化
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 氷柱の中に出現させる
	void StartInIce(const Vector3& icePos);

	// 氷柱に合わせて位置を更新する
	void SetIcePosition(const Vector3& icePos);

	// 氷柱が壊れたので取れる状態にする
	void Release();

	// 取る。取れた場合はtrueを返す
	bool Collect();

	// 消して待機状態に戻す
	void Stop();

	// 現在の状態を取得
	State GetState() const { return state_; }

private:
	// 通常状態の上下移動の時間
	float moveMaxTime_ = 2.0f;

	// 上下移動の振れ幅
	float height_ = 0.5f;

	// 氷柱の位置からの高さ
	float offsetY_ = 3.0f;

	// 1周回転する時間
	float rotateMaxTime_ = 3.0f;

	// 当たり判定の半径
	float colliderRadius_ = 2.0f;

	// 取れる状態になってから消えるまでの時間
	float lifeTime_ = 10.0f;

	// モデルを縦向きに起こすためのX軸回転
	float baseRotateX_ = 1.5707963f;

private:
	// パラメータ機能
	GameEngine::DebugParameter debugParame_{ "HealingHeart" };

	// モデル
	GameEngine::ModelComponent modelComponent_;

	// 球の当たり判定
	GameEngine::SphereCollider collider_;

	// 移動のイージング
	GameEngine::EaseCurve moveCurve_;

	State state_ = State::kInIce;

	// 上下移動の基準の高さ
	float basePosY_ = 0.0f;

	float moveTimer_ = 0.0f;
	float rotateTimer_ = 0.0f;
	float lifeTimer_ = 0.0f;
};
