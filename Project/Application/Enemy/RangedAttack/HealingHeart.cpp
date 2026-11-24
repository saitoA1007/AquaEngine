#include "pch.h"
#include "HealingHeart.h"
#include <numbers>
#include "FPSCounter.h"
#include "EasingManager.h"
#include "Application/CollisionConfig.h"
using namespace GameEngine;

HealingHeart::HealingHeart(GameEngine::Model* model) : modelComponent_(model) {

	// 出現するまでは無効
	isActive_ = false;

	// パラメータ機能
	int index = 0;
	debugParame_.Register("MoveMaxTime", moveMaxTime_, index++);
	debugParame_.Register("Height", height_, index++);
	debugParame_.Register("MoveCurve", moveCurve_, index++);
	debugParame_.Register("OffsetY", offsetY_, index++);
	debugParame_.Register("RotateMaxTime", rotateMaxTime_, index++);
	debugParame_.Register("ColliderRadius", colliderRadius_, index++);
	debugParame_.Register("LifeTime", lifeTime_, index++);
	debugParame_.Register("Scale", modelComponent_.worldTransform_.transform_.scale, index++);
	debugParame_.Register("Color", modelComponent_.materialData_->color, index++);
	debugParame_.Register("BaseRotateX", baseRotateX_, index++);
	debugParame_.Apply();

	// モデルは寝た状態なので縦向きに起こす
	modelComponent_.worldTransform_.transform_.rotate.x = baseRotateX_;

	// 当たり判定(プレイヤーとだけ当たる)
	collider_.SetRadius(colliderRadius_);
	collider_.SetCollisionAttribute(kCollisionAttributeEnemy);
	collider_.SetCollisionMask(kCollisionAttributePlayer);
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kHeart);
	userData.object = this;
	collider_.SetUserData(userData);
	// 氷柱が壊れるまでは当たり判定を無効
	collider_.SetActive(false);
}

void HealingHeart::Initialize() {
	Stop();
}

void HealingHeart::Update() {
	debugParame_.ApplyIfDirty();

	// 氷の中に入っている間は動かさず、氷柱に合わせて位置だけ変える
	if (state_ == State::kReleased) {
		// 回転
		rotateTimer_ += FpsCounter::GetGameDeltaTime() / rotateMaxTime_;
		if (rotateTimer_ >= 1.0f) {
			rotateTimer_ -= 1.0f;
		}
		modelComponent_.worldTransform_.transform_.rotate.y = Lerp(0.0f, std::numbers::pi_v<float> * 2.0f, rotateTimer_);

		// 上下に浮かぶ
		moveTimer_ += FpsCounter::GetGameDeltaTime() / moveMaxTime_;
		if (moveTimer_ >= 1.0f) {
			moveTimer_ -= 1.0f;
		}
		modelComponent_.worldTransform_.transform_.translate.y = basePosY_ + Lerp(-height_, height_, moveTimer_, moveCurve_);

		// 一定時間取られなければ消える
		lifeTimer_ += FpsCounter::GetGameDeltaTime() / lifeTime_;
		if (lifeTimer_ >= 1.0f) {
			Stop();
			return;
		}
	}

	// 縦向きに起こす
	modelComponent_.worldTransform_.transform_.rotate.x = baseRotateX_;

	// 更新処理
	modelComponent_.Update();

	// 当たり判定の位置を更新
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetRadius(colliderRadius_);
}

void HealingHeart::Draw() {
	modelComponent_.DrawRaytracing(renderQueue_);
}

void HealingHeart::StartInIce(const Vector3& icePos) {
	state_ = State::kInIce;
	moveTimer_ = 0.0f;
	rotateTimer_ = 0.0f;
	lifeTimer_ = 0.0f;

	// 前回取れる状態だった時の回転を戻す
	modelComponent_.worldTransform_.transform_.rotate.y = 0.0f;

	SetIcePosition(icePos);
	collider_.SetActive(false);
	isActive_ = true;
}

void HealingHeart::SetIcePosition(const Vector3& icePos) {
	if (state_ != State::kInIce) { return; }

	modelComponent_.worldTransform_.transform_.translate = icePos;
	modelComponent_.worldTransform_.transform_.translate.y += offsetY_;
}

void HealingHeart::Release() {
	if (!isActive_ || state_ != State::kInIce) { return; }

	state_ = State::kReleased;
	moveTimer_ = 0.0f;
	lifeTimer_ = 0.0f;
	basePosY_ = modelComponent_.worldTransform_.transform_.translate.y;

	// 取れるように当たり判定を有効
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetActive(true);
}

bool HealingHeart::Collect() {
	if (!isActive_ || state_ != State::kReleased) { return false; }

	Stop();
	return true;
}

void HealingHeart::Stop() {
	isActive_ = false;
	state_ = State::kInIce;
	collider_.SetActive(false);
}
