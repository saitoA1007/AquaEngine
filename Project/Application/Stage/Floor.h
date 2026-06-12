#pragma once
#include "IGameObject.h"
#include "WorldTransform.h"
#include "Collider.h"
#include "DebugParameter.h"

class Floor : public GameEngine::IGameObject {
public:
	Floor(GameEngine::Model* model);
	~Floor() = default;

	// 初期化
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:
	// ワールド行列を取得
	GameEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// ワールド行列
	GameEngine::WorldTransform worldTransform_;
	// モデル
	GameEngine::Model* model_;

	// aabbの当たり判定
	GameEngine::AABBCollider collider_;

	// 当たり判定のサイズ
	Vector3 colliderSize_ = { 1.0f,1.0f,1.0f };

private:

	// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);
};