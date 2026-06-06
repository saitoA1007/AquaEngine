#pragma once
#include <array>
#include "IGameObject.h"
#include "Collider.h"

#include "IBossState.h"

class BossEnemy : public GameEngine::IGameObject {
public:
	BossEnemy(GameEngine::Model* model, GameEngine::WorldTransform& playerWorld);
	~BossEnemy() = default;

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

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 球の当たり判定
	GameEngine::SphereCollider collider_;

	// モデル
	GameEngine::Model* model_ = nullptr;

	// ワールド行列
	GameEngine::WorldTransform worldTransform_;

	// 状態
	std::array<std::unique_ptr<IBossState>, static_cast<size_t>(BossState::kMaxCount)> statesTable_;
	// 現在の状態
	IBossState* currentState_ = nullptr;
	BossState bossState_ = BossState::kIn;

	// 各状態で共有するデータ
	BossStateCommonData stateCommonData_;

private:

	/// <summary>
	/// 当たり判定
	/// </summary>
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);

	void OnCollisionStay([[maybe_unused]] const GameEngine::CollisionResult& result);
};