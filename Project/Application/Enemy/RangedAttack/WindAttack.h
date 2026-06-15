#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "Collider.h"

class WindAttack : public GameEngine::IGameObject {
public:
	WindAttack(GameEngine::Model* model, Vector3 pos, Vector3 startDir, Vector3 endDir);
	~WindAttack() = default;

	// 初期化
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// モデル
	GameEngine::ModelComponent modelComponent_;

	// obbの当たり判定
	GameEngine::OBBCollider collider_;

	float timer_ = 0.0f;

	Vector3 startDir_ = { 0.0f,0.0f,-1.0f };
	Vector3 endDir_ = { 0.0f,0.0f,-1.0f };

private:

	// 当たり判定
	Vector3 colliderSize_ = {1.0f,1.0f,5.0f};
	Vector3 colliderAnchor_ = { 0.5f,0.5f,0.0f };

	float maxTime_ = 2.0f;

private:

	// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);
};