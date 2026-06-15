#include "IceFall.h"
#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;

IceFall::IceFall(GameEngine::Model* model, Vector3 pos) : modelComponent_(model) {

	// 位置を設定
	modelComponent_.worldTransform_.transform_.translate = pos;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("IceFall");
	debugParame_->Register("ColliderRadius", colliderRadius_);
	debugParame_->Register("InMaxTime", inMaxTime_);
	debugParame_->Register("StartPosY", startPosY_);
	debugParame_->Register("EndPosY", endPosY_);
	debugParame_->Apply();

	// 当たり判定
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetRadius(colliderRadius_);
	collider_.SetCollisionAttribute(kCollisionAttributeEnemy);
	collider_.SetCollisionMask(~kCollisionAttributeEnemy);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kIceFall);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック関数に登録する
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
	});

}

void IceFall::Initialize() {
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetRadius(colliderRadius_);
	modelComponent_.Update();
}

void IceFall::Update() {
	debugParame_->ApplyIfDirty();

	EnterMove();

	modelComponent_.Update();
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetRadius(colliderRadius_);
}

void IceFall::Draw() {
	// 壁を描画
	modelComponent_.Draw(renderQueue_);
}

void IceFall::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

}

void IceFall::EnterMove() {
	if (!isEnterMoveActive_) { return; }

	timer_ += FpsCounter::deltaTime / inMaxTime_;

	modelComponent_.worldTransform_.transform_.translate.y = Lerp(startPosY_, endPosY_, EaseInOut(timer_));

	if (timer_ >= 1.0f) {
		modelComponent_.worldTransform_.transform_.translate.y = endPosY_;
		isEnterMoveActive_ = false;
	}
}