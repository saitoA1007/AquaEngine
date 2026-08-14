#include "Wall.h"
#include "FPSCounter.h"
#include "DebugParameter.h"
#include "Application/CollisionConfig.h"
#include "Application/Player/Player.h"
#include "Application/Enemy/BossEnemy.h"
using namespace GameEngine;

Wall::Wall(GameEngine::Model* model, GameEngine::Model* fractureModel, GameEngine::DebugParameter* parame) : modelComponent_(model), underWallModelComponent_(model),
	destructObject_("Wall", fractureModel, static_cast<uint32_t>(CollisionTypeID::kWall), kCollisionAttributeTerrain) {
	// パラメーター機能を取得
	parame_ = parame;

	std::string subGroup = "Wall";
	int index = 0;
	parame_->Register("ModelScale", modelComponent_.worldTransform_.transform_.scale, index++, subGroup);
	parame_->Register("ColliderSize", colliderSize_, index++, subGroup);
	parame_->Register("MaxHp", maxHp_, index++, subGroup);
	parame_->Register("RespawnTime", respawnTime_, index++, subGroup);

	modelComponent_.materialData_->color.w = 0.8f;

	// 当たり判定
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.UpdateOrientationsFromRotate(modelComponent_.worldTransform_.transform_.rotate);
	collider_.SetCollisionAttribute(kCollisionAttributeTerrain);
	collider_.SetCollisionMask(~kCollisionAttributeTerrain);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kWall);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック関数に登録する
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
	});

	// 参照するマテリアルを変更
	modelComponent_.SetBufferMaterial(0, iceMaterial_.GetMaterialSrvIndex());
	modelComponent_.SetHitGroup(1);

	underWallModelComponent_.SetBufferMaterial(0, iceMaterial_.GetMaterialSrvIndex());
	underWallModelComponent_.SetHitGroup(1);
}

void Wall::SetParameter(const Transform& transform) {
	// 位置を取得
	modelComponent_.worldTransform_.transform_.translate = transform.translate;
	modelComponent_.worldTransform_.transform_.rotate = transform.rotate;
	modelComponent_.worldTransform_.transform_.scale = { 2.0f,2.0f,1.5f };

	// 下に存在する壁を設置する
	underWallModelComponent_.worldTransform_.transform_ = modelComponent_.worldTransform_.transform_;
	underWallModelComponent_.worldTransform_.transform_.translate.y = -2.0f;
	underWallModelComponent_.worldTransform_.transform_.scale * 0.8f;

	// 初期化
	Initialize();
}

void Wall::Initialize() {
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.UpdateOrientationsFromRotate(modelComponent_.worldTransform_.transform_.rotate);
	modelComponent_.Update();
	underWallModelComponent_.Update();
}

void Wall::Update() {

	if (!isBreakIce_) { return; }
	respawnTimer_ += FpsCounter::gameDeltaTime / respawnTime_;

	// リスポーン時間を超えたら、復活する
	if (respawnTimer_ >= 1.0f) {
		isBreakIce_ = false;
		respawnTimer_ = 0.0f;
		modelComponent_.worldTransform_.transform_.scale.z = 1.5f;
		currentHp_ = maxHp_;
	}

	// モデルの更新処理
	modelComponent_.Update();

	// 破片の更新処理
	destructObject_.Update();
}

void Wall::Draw() {

	// 下に存在する壁を設置する
	underWallModelComponent_.DrawRaytracing(renderQueue_);

	if (isBreakIce_) { return; }

	// 壁を描画
	modelComponent_.DrawRaytracing(renderQueue_);

	// 破片を描画
	destructObject_.Draw();
}

void Wall::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

	bool isPlayer = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kPlayer));
	bool isBoss = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kBoss));

	Player* player = nullptr;
	BossEnemy* boss = nullptr;

	if (isPlayer) { 
		player = result.userData.As<Player>();
	}

	if (isBoss) {
		boss = result.userData.As<BossEnemy>();
	}

	// Hpを削る
	if (player != nullptr) {

		if (player->IsHitWall()) {
			player->SetIsHitWall(false);
			Vector3 velocity = player->GetVelocity();
			velocity.y = 0.0f;
			// 現在の速度の割合を取得
			float ratio = velocity.Length() / player->GetRushMaxSpeed();

			if (ratio <= 0.3f) {
				currentHp_ -= 1;
			} else if (ratio <= 0.7f) {
				currentHp_ -= 2;
			} else {
				currentHp_ -= 3;
			}
		}
	} else if (boss != nullptr) {

		BossBattleState battleState = boss->GetBattleState();

		// ボスが突進状態であれば
		if (battleState == BossBattleState::kRushAttack) {
			// ボスの場合、固定ダメージ
			currentHp_ -= 2;
		}
	}

	// hpによって形を帰る
	if (currentHp_ == 2) {
		modelComponent_.worldTransform_.transform_.scale.z = 1.0f;
	} else if (currentHp_ == 1) {
		modelComponent_.worldTransform_.transform_.scale.z = 0.5f;
	}else if (currentHp_ <= 0) {
		currentHp_ = 0;
		isBreakIce_ = true;
	}

	modelComponent_.worldTransform_.UpdateTransformMatrix();
}