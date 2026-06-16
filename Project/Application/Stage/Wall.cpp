#include "Wall.h"
#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
using namespace GameEngine;

Wall::Wall(GameEngine::Model* model, float& respawnTime, int32_t& maxHp) : respawnTime_(respawnTime), maxHp_(maxHp), modelComponent_(model) {
	
	modelComponent_.materialData_->color.w = 0.8f;

	// 当たり判定
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(modelComponent_.worldTransform_.transform_.scale);
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
}

void Wall::SetParameter(const Transform& transform) {
	// 位置を取得
	modelComponent_.worldTransform_.transform_ = transform;
	modelComponent_.Update();

	// 当たり判定の更新
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(modelComponent_.worldTransform_.transform_.scale + Vector3(0.0f, 100.0f, 0.0f));
	collider_.UpdateOrientationsFromRotate(modelComponent_.worldTransform_.transform_.rotate);
}

void Wall::Initialize() {
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(modelComponent_.worldTransform_.transform_.scale + Vector3(0.0f,100.0f,0.0f));
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