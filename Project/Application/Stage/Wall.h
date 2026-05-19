#pragma once
#include "IGameObject.h"
#include "WorldTransform.h"
#include "Collider.h"

class Wall : public GameEngine::IGameObject {
public:
	Wall(GameEngine::Model* model, float& respawnTime, int32_t& maxHp);
	~Wall() = default;

	void SetParameter(const Transform& transform);

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
	// ワールド行列
	GameEngine::WorldTransform worldTransform_;
	// モデル
	GameEngine::Model* model_;

	// リスポーン時間
	float& respawnTime_;
	// 最大hp
	int32_t& maxHp_;

	float respawnTimer_ = 0.0f;

	// 現在のhp
	int32_t currentHp_ = 1;

	// 生存フラグ
	bool isAlive_ = true;

	// obbの当たり判定
	GameEngine::OBBCollider collider_;

private:

	// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);
};