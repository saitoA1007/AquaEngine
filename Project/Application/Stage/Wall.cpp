#include "Wall.h"
#include "FPSCounter.h"
#include "DebugParameter.h"
#include "Application/CollisionConfig.h"

using namespace GameEngine;

Wall::Wall(GameEngine::Model* model, GameEngine::DebugParameter* parame) : modelComponent_(model) {
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
}

void Wall::SetParameter(const Transform& transform) {
	// 位置を取得
	modelComponent_.worldTransform_.transform_.translate = transform.translate;
	modelComponent_.worldTransform_.transform_.rotate = transform.rotate;
	modelComponent_.worldTransform_.transform_.scale = { 2.0f,2.0f,1.5f };

	// 初期化
	Initialize();
}

void Wall::Initialize() {
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.UpdateOrientationsFromRotate(modelComponent_.worldTransform_.transform_.rotate);
	modelComponent_.Update();
}

void Wall::Update() {

	if (currentHp_ <= 0) {
		isAlive_ = false;
	}

	if (isAlive_) { return; }
	respawnTimer_ += FpsCounter::gameDeltaTime / respawnTime_;

	// リスポーン時間を超えたら、復活する
	if (respawnTimer_ >= 1.0f) {
		isAlive_ = true;
		respawnTimer_ = 0.0f;
		currentHp_ = maxHp_;
	}

	modelComponent_.Update();
}

void Wall::Draw() {
	// 壁を描画
	modelComponent_.DrawRaytracing(renderQueue_);
}

void Wall::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

}