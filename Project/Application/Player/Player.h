#pragma once
#include "IGameObject.h"
#include "WorldTransform.h"
#include "InputCommand.h"
#include "DebugParameter.h"
#include "Collider.h"

#include "PlayerAction.h"
#include "Application/Camera/CameraController.h"

class Player : public GameEngine::IGameObject {
public:
	Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model);
	~Player() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

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

private:
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

private:

	// 制限
	void ApplyClamp();

	// 回転の更新
	void UpdateRotation();

	// 当たり判定
	void OnCollisionStay(const GameEngine::CollisionResult& result);
};