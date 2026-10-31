#pragma once
#include "IGameObject.h"
#include "WorldTransform.h"
#include "Collider.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "IceMaterial.h"
#include "DestructibleObject.h"

class Floor : public GameEngine::IGameObject {
public:
	Floor(GameEngine::Model* model, GameEngine::Model* fractureModel, uint32_t iceNormalGH, uint32_t iceHeightGH, uint32_t terrainGH, uint32_t terrainNormalGH);
	~Floor() = default;

	// 初期化
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:
	// ワールド行列を取得
	GameEngine::WorldTransform& GetWorldTransform() { return iceModelComponent_.worldTransform_; }

	/// <summary>
	/// 床の上層部を壊す
	/// </summary>
	/// <param name="worldPos">壊す位置</param>
	void Break(const Vector3& worldPos);

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 氷のモデル
	GameEngine::ModelComponent iceModelComponent_;

	// 氷のマテリアル
	GameEngine::IceMaterial iceMaterial_;

	// 地面のモデル
	GameEngine::ModelComponent terrainModelComponent_;

	// aabbの当たり判定
	GameEngine::AABBCollider collider_;

	// 床の破片のモデル
	GameEngine::DestructibleObject destructObject_;

	// 当たり判定のサイズ
	Vector3 colliderSize_ = { 1.0f,1.0f,1.0f };
	// アンカーポイント
	Vector3 colliderAnchor_ = { 0.5f,0.5f,0.5f };

	// 上層部の大きさ(破壊モデルは横50×厚さ2×奥行50)
	Vector3 fractureScale_ = { 1.0f,0.2f,1.0f };
	// 外側の氷の平面を下げる量
	float outerIceOffsetY_ = 0.45f;

	// 落下攻撃で与えるダメージ
	float breakDamage_ = 1000.0f;
	// 落下攻撃で壊す範囲
	float breakRadius_ = 4.0f;

	// 壊れてから元に戻るまでの時間
	float respawnTime_ = 5.0f;
	float respawnTimer_ = 0.0f;
	// 壊れているか
	bool isBroken_ = false;

private:

	// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);
};