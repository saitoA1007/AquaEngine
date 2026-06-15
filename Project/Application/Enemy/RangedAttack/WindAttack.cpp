#include "WindAttack.h"
#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "MyMath.h"
using namespace GameEngine;

WindAttack::WindAttack(GameEngine::Model* model, Vector3 pos, Vector3 startDir, Vector3 endDir, float maxTime) : modelComponent_(model) {

	modelComponent_.worldTransform_.transform_.translate = pos;
	startDir_ = startDir;
	endDir_ = endDir;
	maxTime_ = maxTime;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("WindAttack");
	debugParame_->Register("ColliderRadius", colliderSize_);
	debugParame_->Apply();

	// 当たり判定
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.SetAnchor(colliderAnchor_);
	collider_.UpdateOrientationsFromRotate(modelComponent_.worldTransform_.transform_.rotate);
	collider_.SetCollisionAttribute(kCollisionAttributeEnemy);
	collider_.SetCollisionMask(~kCollisionAttributeEnemy);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kWind);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック関数に登録する
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
		});

}

void WindAttack::Initialize() {
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.SetAnchor(colliderAnchor_);
	collider_.UpdateOrientationsFromRotate(modelComponent_.worldTransform_.transform_.rotate);
	modelComponent_.Update();
}

void WindAttack::Update() {
	debugParame_->ApplyIfDirty();

	timer_ += FpsCounter::deltaTime / maxTime_;

	// 回転
	Vector3 dir = Slerp(startDir_, endDir_, timer_);

	// 方向から角度を取得
	modelComponent_.worldTransform_.transform_.rotate = Math::DirectionToEuler(dir);

	if (timer_ >= 1.0f) {
		isDead_ = true;
	}

	modelComponent_.Update();
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.UpdateOrientationsFromRotate(modelComponent_.worldTransform_.transform_.rotate);
}

void WindAttack::Draw() {
	// 壁を描画
	modelComponent_.Draw(renderQueue_);
}

void WindAttack::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

}