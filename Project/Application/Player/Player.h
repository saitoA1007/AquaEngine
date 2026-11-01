#pragma once
#include <vector>
#include "IGameObject.h"
#include "WorldTransform.h"
#include "InputCommand.h"
#include "DebugParameter.h"
#include "Collider.h"

#include "PlayerAction.h"
#include "Application/Camera/CameraController.h"
#include "PlayerEffectManager.h"

// 前方宣言
class PlayerEffectManager;

class Player : public GameEngine::IGameObject {
public:
	Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model, GameEngine::AnimationManager* animationManager, PlayerEffectManager* effectManager);
	~Player();

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	/// <summary>
	/// ワールド行列を取得
	/// </summary>
	/// <returns></returns>
	GameEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

	/// <summary>
	/// プレイヤーの位置を取得
	/// </summary>
	/// <returns></returns>
	Vector3 GetPlayerPos() { return worldTransform_.GetWorldPosition(); }

	// カメラのワールド行列を取得
	void SetCamera(CameraController* camera) {
		camera_ = camera;
	}

	// 現在のHpを取得
	int32_t GetCurrentHp() const { return playerStatus_.GetCurrentHp(); }

	// 最大のHpを取得
	int32_t GetMaxHp() const { return playerStatus_.GetMaxHp(); }

	// 現在の状態を取得
	PlayerState GetCurrentState() const { return commonData_.state; }

	// 攻撃力
	float GetDamage() const { return playerAttackDownAction_.GetAttackDownPower(); }

	// 突進の最大速度
	float GetRushMaxSpeed() const { return attackRushAction_.GetRushMaxSpeed(); }

	// 速度を取得
	Vector3 GetVelocity() const { return commonData_.velocity; }

	bool IsHitWall() const { return bounceAction_.IsHitWall(); }
	void SetIsHitWall(bool isHitWall) { bounceAction_.SetIsHitWall(isHitWall); }

	// 描画の有効を設定
	void SetIsDraw(bool isDraw) {
		isDraw_ = isDraw;
	}

	// ヒットエフェクトを開始
	void StartHitEffect(Vector3 pos);

	/// <summary>
	/// コントローラーを振動させる
	/// </summary>
	/// <param name="intensity">振動の強さ(0~1)</param>
	/// <param name="duration">振動する時間(秒)</param>
	void StartVibration(float intensity, float duration);

private:
	// actions_が自身のメンバを指すためコピー禁止
	Player(const Player&) = delete;
	Player& operator=(const Player&) = delete;

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;
	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;
	// モデル
	GameEngine::Model* model_ = nullptr;
	// カメラ機能
	CameraController* camera_ = nullptr;

	// ワールド行列
	GameEngine::WorldTransform worldTransform_;

	// 球の当たり判定
	GameEngine::SphereCollider collider_;

	// アニメーション管理
	std::unique_ptr<PlayerAnimator> animator_;

	// 仮のプレイヤーのエフェクト管理システム
	PlayerEffectManager* effectManager_ = nullptr;

	// 表示フラグ
	bool isDraw_ = true;

	// 敵に攻撃した時の揺れの強さ
	float attackShakeIntensity_ = 0.3f;
	// 敵に攻撃した時の揺れる時間
	float attackShakeDuration_ = 0.5f;
	// ダメージを受けた時の揺れの強さ
	float damageShakeIntensity_ = 0.3f;
	// ダメージを受けた時の揺れる時間
	float damageShakeDuration_ = 0.6f;

	// 敵に攻撃した時の振動の強さ
	float attackVibrationIntensity_ = 0.3f;
	// 敵に攻撃した時の振動する時間
	float attackVibrationDuration_ = 0.25f;

	// 歩きエフェクトを出す最低の速さ
	float moveEffectMinSpeed_ = 2.0f;

	// 振動の残り時間
	float vibrationTimer_ = 0.0f;
	// 現在の振動の強さ
	float vibrationIntensity_ = 0.0f;

private: 
	// プレイヤー
	PlayerCommonData commonData_;

	// 移動アクション
	PlayerMoveAction moveAction_;
	// 突進アクション
	PlayerAttackRushAction attackRushAction_;
	// 跳ね返りアクション
	PlayerBounceAction bounceAction_;
	// 落下攻撃アクション
	PlayerAttackDownAction playerAttackDownAction_;

	// プレイヤーが受ける物理
	PlayerPhysics playerPhysics_;

	// プレイヤーの状態
	PlayerStatus playerStatus_;

	// 一括処理用のアクション一覧
	std::vector<IPlayerAction*> actions_;

private:

	// 制限
	void ApplyClamp();

	// ダメージを受ける
	void TakeDamage(int32_t damage);

	// 振動の更新
	void UpdateVibration();

	// 振動を止める
	void StopVibration();

	// 回転の更新
	void UpdateRotation();

	// 歩きエフェクトの更新
	void UpdateMoveEffect();

	// 当たり判定
	void OnCollisionStay(const GameEngine::CollisionResult& result);
};
