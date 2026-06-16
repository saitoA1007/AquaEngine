#include "Floor.h"
#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
using namespace GameEngine;

Floor::Floor(GameEngine::Model* model) {
	model_ = model;
	// ワールド行列を初期化
	worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Floor");
	debugParame_->RegisterWorld("world", worldTransform_);
	debugParame_->Register("ColliderSize", colliderSize_);
	debugParame_->Register("ColliderAnchor", colliderAnchor_);
	debugParame_->Apply();

	// 当たり判定
	collider_.SetWorldPosition(worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.SetAnchorPoint(colliderAnchor_);
	collider_.SetCollisionAttribute(kCollisionAttributeTerrain);
	collider_.SetCollisionMask(~kCollisionAttributeTerrain);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kGround);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック関数に登録する
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
	});
}

void Floor::Initialize() {
	collider_.SetWorldPosition(worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.SetAnchorPoint(colliderAnchor_);
	worldTransform_.UpdateTransformMatrix();
}

void Floor::Update() {
	debugParame_->ApplyIfDirty();

	worldTransform_.UpdateTransformMatrix();

	collider_.SetWorldPosition(worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.SetAnchorPoint(colliderAnchor_);
}

void Floor::Draw() {
	// 壁を描画
	renderQueue_->SubmitRaytracingModel(model_, worldTransform_);
}

void Floor::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

}