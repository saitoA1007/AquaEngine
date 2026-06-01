#define NOMINMAX
#include "Player.h"
#include <algorithm>
#include "LogManager.h"
#include "EasingManager.h"
#include "MyMath.h"
#include "FPSCounter.h"
#include "Model.h"
#include "Application/CollisionConfig.h"
using namespace GameEngine;

Player::Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model) {
	inputCommand_ = inputCommand;
	model_ = model;
	
	// ワールド行列を初期化
	worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{-2.0f,1.0f,0.0f} });

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Player");
	// 値を登録
	moveAction_.RegisterParameter(debugParame_.get());
	bounceAction_.RegisterParameter(debugParame_.get());

	// 当たり判定の実装
	collider_.SetRadius(1.0f);
	collider_.SetWorldPosition(worldTransform_.transform_.translate);
	collider_.SetCollisionAttribute(kCollisionAttributePlayer);
	collider_.SetCollisionMask(~kCollisionAttributePlayer);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::Player);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック登録
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionStay(result); 
		});


	// 移動アクション
	moveAction_.Initialize(&commonData_, inputCommand);
	// 跳ね返りアクション
	bounceAction_.Initialize(&commonData_);
	// 突進アクション
	attackRushAction_.Initialize(&commonData_, inputCommand);
	// 重力
	playerPhysics_.Initialize(&commonData_);
}

void Player::Initialize() {
	// ステータスを初期化
	commonData_ = PlayerCommonData{};
	commonData_.currentYaw = std::atan2f(commonData_.currentDir.x, commonData_.currentDir.z);

	// 位置を初期化
	worldTransform_.transform_.translate = { 0.0f,0.0f,0.0f };
}

void Player::Update() {
	// 値の適応
	debugParame_->ApplyIfDirty();

	// カメベクトルを更新
	moveAction_.UpdateCameraBasis(camera_->GetWorldMatrix());
	// 移動操作
	moveAction_.ProcessMoveInput();
	// 突進操作
	attackRushAction_.ProcessInput();

	// 受ける物理を更新
	playerPhysics_.Update();

	// 移動を適応
	worldTransform_.transform_.translate += commonData_.velocity * FpsCounter::deltaTime;

	// プレイヤーを移動範囲に制限
	worldTransform_.transform_.translate.x = std::clamp(worldTransform_.transform_.translate.x, -9.0f, 9.0f);
	worldTransform_.transform_.translate.z = std::clamp(worldTransform_.transform_.translate.z, -9.0f, 9.0f);

	// 地面
	if (worldTransform_.transform_.translate.y <= 0.0f) {
		worldTransform_.transform_.translate.y = 0.0f;
		commonData_.velocity.y = 0.0f;
	}

	// 現在向いている角度を更新
	commonData_.currentDir = Vector3(commonData_.velocity.x, 0.0f, commonData_.velocity.z);
	commonData_.currentDir.Normalize();

	// 回転を適応
	worldTransform_.transform_.rotate = Math::DirectionToEuler(commonData_.currentDir);

	// 目標方向を取得
	Vector3 targetDir = { 0.0f, 0.0f, 0.0f };
	if (commonData_.targetDir.x != 0.0f || commonData_.targetDir.z != 0.0f) {
		targetDir = commonData_.targetDir;
	} else {
		targetDir = commonData_.currentDir;
	}
	
	// 回転の更新
	if (targetDir.x != 0.0f || targetDir.z != 0.0f) {
		targetDir.y = 0.0f;
		targetDir.Normalize();

		// 目標ヨー角を計算
		float targetYaw = std::atan2f(targetDir.x, targetDir.z);

		// 最短経路で角度補間をして現在の角度を求める
		float kRotationLerpSpeed_ = 10.0f;
		float maxStep = kRotationLerpSpeed_ * FpsCounter::deltaTime;
		commonData_.currentYaw = Math::LerpShortAngle(commonData_.currentYaw, targetYaw, maxStep);

		// ヨー角をワールドトランスフォームに反映
		worldTransform_.transform_.rotate.y = commonData_.currentYaw;
		// 現在の角度を更新
		commonData_.currentDir = Math::YawToDirection(commonData_.currentYaw);
	}

	// 行列の更新
	worldTransform_.UpdateTransformMatrix();
	// 当たり判定の更新
	collider_.SetWorldPosition(worldTransform_.GetWorldPosition());

	// 更新
	attackRushAction_.Update();
}

void Player::Draw() {

	// モデル描画
	renderQueue_->SubmitRaytracingModel(model_, worldTransform_);
}

void Player::OnCollisionStay(const GameEngine::CollisionResult& result) {

	bool isWall = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::Wall));
	
	// 壁の衝突処理
	if (isWall) {
		//Log("Player is hit Wall");
		bounceAction_.WallBounce(worldTransform_.transform_.translate, result.contactNormal, result.penetrationDepth);
	}
}