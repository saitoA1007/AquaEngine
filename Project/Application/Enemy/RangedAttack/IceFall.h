#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "Collider.h"

class IceFall : public GameEngine::IGameObject {
public:
	IceFall(GameEngine::Model* model, Vector3 pos);
	~IceFall() = default;

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

	// 現在のhp
	int32_t currentHp_ = 1;

	// 生存フラグ
	bool isAlive_ = true;

	// 球の当たり判定
	GameEngine::SphereCollider collider_;

	float timer_ = 0.0f;

	bool isEnterMoveActive_ = true;

private:

	// 半径
	float colliderRadius_ = 1.0f;

	float inMaxTime_ = 1.0f;

	float startPosY_ = -2.0f;
	float endPosY_ = 0.0f;

private:

	// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);

	// 入りの動き
	void EnterMove();
};