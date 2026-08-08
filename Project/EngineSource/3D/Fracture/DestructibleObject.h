#pragma once
#include <memory>
#include "IGameObject.h"
#include "Collider.h"
#include "Model.h"
#include "WorldTransform.h"
#include "IceMaterial.h"
#include "DebugParameter.h"
#include "FractureDamageController.h"

namespace GameEngine {

	/// <summary>
	/// 破壊オブジェクト
	/// </summary>
	class DestructibleObject : public IGameObject {
	public:
		DestructibleObject(std::string name, Model* model, uint32_t colliderId, uint32_t colliderAttribute);
		~DestructibleObject() = default;

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		// 描画処理
		void Draw() override;

	public:

		// ワールド行列
		WorldTransform worldTransform_;

	private:
		// 名前
		std::string name_ = "noName";

		// モデル
		Model* model_ = nullptr;

		// 氷のマテリアル
		IceMaterial iceMaterial_;

		// aabbの当たり判定
		AABBCollider collider_;

		// コライダー生成時に渡す識別情報
		uint32_t colliderId_ = 0;
		uint32_t colliderAttribute_ = 0;

		// ダメージ判定・破砕伝播・爆発・ひび割れ物理
		FractureDamageController damageController_;

		// パラメータ機能
		std::unique_ptr<DebugParameter> debugParameter_;

		// 一時的なテスト用の項目
		Vector3 colliderSize_ = { 2.5f, 2.5f, 2.5f };
		float testDamageAmount_ = 2.0f;
		float testCraterRadius_ = 2.0f;
		int testPlaneCount_ = 8;

	private:
		// 当たり判定のコールバック関数
		void OnCollisionEnter(const GameEngine::CollisionResult& result);
	};
}
