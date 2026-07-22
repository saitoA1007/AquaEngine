#pragma once
#include "IGameObject.h"
#include "Collider.h"
#include "Model.h"
#include "FractureInstance.h"

namespace GameEngine {

	class DestructibleObject : public IGameObject {
	public:
		DestructibleObject(Model* model, uint32_t colliderId, uint32_t colliderAttribute);
		~DestructibleObject() = default;

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		// 描画処理
		void Draw() override;

	private:
		// モデル
		Model* model_ = nullptr;

		// aabbの当たり判定
		AABBCollider collider_;

		// 破片
		FractureInstance fractureInstance_;

	private:

		// 当たり判定のコールバック関数
		void OnCollisionEnter(const GameEngine::CollisionResult& result);

		void ApplyDamage(const Vector3& impactPos, float damageRadius);
	};
}

