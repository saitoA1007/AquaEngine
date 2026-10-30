#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "IceMaterial.h"
#include "Collider.h"
#include "DestructibleObject.h"

class IceFall : public GameEngine::IGameObject {
public:
	IceFall(GameEngine::Model* model, GameEngine::Model* fractureModel, Vector3 pos, int32_t& iceFallCurrentNum);
	~IceFall();

	// 初期化
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	/// <summary>
	/// 破壊する
	/// </summary>
	/// <param name="hitPos">当たった位置</param>
	/// <param name="hitDir">当たった方向</param>
	void Break(const Vector3& hitPos, const Vector3& hitDir);

	// 破壊されているか
	bool IsBreak() const { return isBreak_; }

	// 出現させる
	void Start(Vector3 pos);

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// モデル
	//GameEngine::ModelComponent modelComponent_;

	// 氷のマテリアル
	GameEngine::IceMaterial iceMaterial_;

	// 破片のオブジェクト
	GameEngine::DestructibleObject destructibleObject_;

	// 現在の氷柱の数
	int32_t& iceFallCurrentNum_;

	// 現在のhp
	int32_t currentHp_ = 1;

	// 球の当たり判定
	GameEngine::SphereCollider collider_;

	float timer_ = 0.0f;

	bool isEnterMoveActive_ = false;

	bool isBreak_ = false;

private:

	// 半径
	float colliderRadius_ = 2.0f;

	float inMaxTime_ = 1.0f;

	float startPosY_ = -2.0f;
	float endPosY_ = 0.0f;

	// 破壊された時に与えるダメージ
	float breakDamage_ = 1000.0f;
	// 破壊された時にダメージを与える範囲
	float breakCraterRadius_ = 100.0f;
	// 破壊されてから消えるまでの時間
	float breakDisappearTime_ = 2.0f;

private:

	// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);

	// 入りの動き
	void EnterMove();
};