#include "Wall.h"
#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
using namespace GameEngine;

Wall::Wall(GameEngine::Model* model, float& respawnTime, int32_t& maxHp) : respawnTime_(respawnTime), maxHp_(maxHp) {
	model_ = model;
	// ワールド行列を初期化
	worldTransform_.Initialize({{1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}});

	// 当たり判定
	collider_.SetWorldPosition(worldTransform_.transform_.translate);
	collider_.SetSize(worldTransform_.transform_.scale);
	collider_.UpdateOrientationsFromRotate(worldTransform_.transform_.rotate);
	collider_.SetCollisionAttribute(kCollisionAttributeTerrain);
	collider_.SetCollisionMask(~kCollisionAttributeTerrain);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::Wall);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック関数に登録する
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
		});
}

void Wall::SetParameter(const Transform& transform) {
	// 位置を取得
	worldTransform_.transform_ = transform;
	worldTransform_.UpdateTransformMatrix();

	// 当たり判定の更新
	collider_.SetWorldPosition(worldTransform_.transform_.translate);
	collider_.SetSize(worldTransform_.transform_.scale + Vector3(0.0f, 100.0f, 0.0f));
	collider_.UpdateOrientationsFromRotate(worldTransform_.transform_.rotate);
}

void Wall::Initialize() {
	collider_.SetWorldPosition(worldTransform_.transform_.translate);
	collider_.SetSize(worldTransform_.transform_.scale + Vector3(0.0f,100.0f,0.0f));
	collider_.UpdateOrientationsFromRotate(worldTransform_.transform_.rotate);
	worldTransform_.UpdateTransformMatrix();
}

void Wall::Update() {

	if (currentHp_ <= 0) {
		isAlive_ = false;
	}

	if (isAlive_) { return; }
	respawnTimer_ += FpsCounter::deltaTime / respawnTime_;

	// リスポーン時間を超えたら、復活する
	if (respawnTimer_ >= 1.0f) {
		isAlive_ = true;
		respawnTimer_ = 0.0f;
		currentHp_ = maxHp_;
	}
}

void Wall::Draw() {
	// 壁を描画
	renderQueue_->SubmitRaytracingModel(model_, worldTransform_);
}

void Wall::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

}