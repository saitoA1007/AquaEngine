#include "BossEnemy.h"

// 各状態
#include "State/BossStateIn.h"
#include "State/BossStateBattle.h"
#include "State/BossStateOut.h"

#include "Application/CollisionConfig.h"

using namespace GameEngine;

BossEnemy::BossEnemy(GameEngine::Model* model) {

	model_ = model;

	// 初期化
	worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("BossEnemy");

	// 共通データ設定
	stateCommonData_.worldTransform = &worldTransform_;
	stateCommonData_.debugParame = debugParame_.get();

	// 状態の生成
	statesTable_[static_cast<size_t>(BossState::In)] = std::make_unique<BossStateIn>(stateCommonData_);
	statesTable_[static_cast<size_t>(BossState::Battle)] = std::make_unique<BossStateBattle>(stateCommonData_);
	statesTable_[static_cast<size_t>(BossState::Out)] = std::make_unique<BossStateOut>(stateCommonData_);

	// 最初の状態を設定する
	bossState_ = BossState::In;
	currentState_ = statesTable_[static_cast<size_t>(BossState::In)].get();
	currentState_->Enter();

	// 当たり判定を設定
	collider_.SetRadius(1.0f);
	collider_.SetWorldPosition(worldTransform_.transform_.translate);
	collider_.SetCollisionAttribute(kCollisionAttributeEnemy);
	collider_.SetCollisionMask(~kCollisionAttributeEnemy);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::Boss);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック登録
	collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
	});

	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionStay(result);
	});
}

void BossEnemy::Initialize() {
	worldTransform_.transform_.translate = { 0.0f,5.0f,0.0f };
}

void BossEnemy::Update() {
	// 状態変更が有効であれば、切り替える
	if (stateCommonData_.bossStateRequest) {
		currentState_->Exit();
		bossState_ = stateCommonData_.bossStateRequest.value();
		currentState_ = nullptr;
		currentState_ = statesTable_[static_cast<size_t>(*stateCommonData_.bossStateRequest)].get();
		currentState_->Enter();
		stateCommonData_.bossStateRequest = std::nullopt;
	}

	// 現在の状態の更新処理
	currentState_->Update();

	// 行列の更新
	worldTransform_.UpdateTransformMatrix();

	// 当たり判定の位置を更新
	collider_.SetWorldPosition(worldTransform_.GetWorldPosition());
}

void BossEnemy::Draw() {

	// 描画
	renderQueue_->SubmitRaytracingModel(model_, worldTransform_);
}

void BossEnemy::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

}

void BossEnemy::OnCollisionStay([[maybe_unused]] const GameEngine::CollisionResult& result) {


}