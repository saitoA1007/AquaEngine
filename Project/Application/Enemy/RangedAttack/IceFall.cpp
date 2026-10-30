#include "pch.h"
#include "IceFall.h"
#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;

IceFall::IceFall(GameEngine::Model* model, GameEngine::Model* fractureModel, Vector3 pos, int32_t& iceFallCurrentNum) :
	iceFallCurrentNum_(iceFallCurrentNum),
	destructibleObject_("IceFall", fractureModel, static_cast<uint32_t>(CollisionTypeID::kIceFall), kCollisionAttributeEnemy) {

	// 設定だけするので有効フラグはオフにする
	isActive_ = false;

	// 初期位置を設定
	destructibleObject_.worldTransform_.transform_.translate = Vector3(0.0f,100.0f,0.0f);

	// 当たり判定を無効
	destructibleObject_.SetIsColliderActive(false);

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("IceFall");
	debugParame_->Register("ColliderRadius", colliderRadius_);
	debugParame_->Register("InMaxTime", inMaxTime_);
	debugParame_->Register("StartPosY", startPosY_);
	debugParame_->Register("EndPosY", endPosY_);
	debugParame_->Register("Scale", destructibleObject_.worldTransform_.transform_.scale);
	debugParame_->Register("BreakDamage", breakDamage_);
	debugParame_->Register("BreakCraterRadius", breakCraterRadius_);
	debugParame_->Register("BreakDisappearTime", breakDisappearTime_);
	debugParame_->Apply();

	// 当たり判定
	collider_.SetWorldPosition(destructibleObject_.worldTransform_.transform_.translate);
	collider_.SetRadius(colliderRadius_);
	collider_.SetCollisionAttribute(kCollisionAttributeEnemy);
	collider_.SetCollisionMask(~kCollisionAttributeEnemy);
	// 出現するまでは当たり判定を無効にしておく
	collider_.SetActive(false);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kIceFall);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック関数に登録する
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
	});

	// 参照するマテリアルを変更
	//modelComponent_.SetBufferMaterial(0, iceMaterial_.GetMaterialSrvIndex());
	//modelComponent_.SetHitGroup(1);
}

IceFall::~IceFall() {
	// 使い回すため、数の管理はStartとUpdateでおこなう
}

void IceFall::Initialize() {
	collider_.SetWorldPosition(destructibleObject_.worldTransform_.transform_.translate);
	collider_.SetRadius(colliderRadius_);
	destructibleObject_.Update();
}

void IceFall::Update() {
	debugParame_->ApplyIfDirty();

	EnterMove();

	//modelComponent_.Update();
	collider_.SetWorldPosition(destructibleObject_.worldTransform_.transform_.translate);
	collider_.SetRadius(colliderRadius_);

	// 破片の更新処理
	destructibleObject_.Update();

	if (isBreak_) {
		timer_ += FpsCounter::gameDeltaTime / breakDisappearTime_;
	
		// 破片が落ちきったら非表示にして、次の出現まで待機させる
		if (timer_ >= 1.0f) {
			isActive_ = false;
			isBreak_ = false;
			timer_ = 0.0f;
			iceFallCurrentNum_ -= 1;
		}
	}
}

void IceFall::Draw() {
	// 壁を描画
	//modelComponent_.DrawRaytracing(renderQueue_);

	// 破片の描画
	destructibleObject_.Draw();
}

void IceFall::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

	//destructibleObject_.damageAmount_ = 1000.0f;
	//destructibleObject_.craterRadius_ = 100.0f;
	//
	//destructibleObject_.OnCollisionEnter(result);
}

void IceFall::Break(const Vector3& hitPos, const Vector3& hitDir) {
	if (isBreak_) { return; }

	isBreak_ = true;
	isEnterMoveActive_ = false;
	timer_ = 0.0f;

	// 破片が残っている間に再び当たらないよう当たり判定を無効にする
	collider_.SetActive(false);

	// 大きなダメージを与えて全体を砕く
	destructibleObject_.ApplyDamage(hitPos, hitDir, breakDamage_, breakCraterRadius_);
}

void IceFall::Start(Vector3 pos) {
	// 現在数を増やす
	iceFallCurrentNum_ += 1;

	// 前回の破片を消して無傷の状態に戻す
	destructibleObject_.ResetToIntact();

	// 状態をリセット
	isBreak_ = false;
	isEnterMoveActive_ = true;
	timer_ = 0.0f;

	pos.y = startPosY_;
	destructibleObject_.worldTransform_.transform_.translate = pos;

	collider_.SetWorldPosition(pos);
	collider_.SetActive(true);
	isActive_ = true;
}

void IceFall::EnterMove() {
	if (!isEnterMoveActive_) { return; }

	timer_ += FpsCounter::gameDeltaTime / inMaxTime_;

	destructibleObject_.worldTransform_.transform_.translate.y = Lerp(startPosY_, endPosY_, timer_, EaseType::kEaseInOutQuad);

	if (timer_ >= 1.0f) {
		destructibleObject_.worldTransform_.transform_.translate.y = endPosY_;
		isEnterMoveActive_ = false;
		timer_ = 0.0f;
	}
}