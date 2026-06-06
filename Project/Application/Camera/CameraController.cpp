#include "CameraController.h"
#include "MyMath.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;

CameraController::CameraController(GameEngine::InputCommand* inputCommand, const GameEngine::WorldTransform* targetWorld, const GameEngine::WorldTransform* playerWorld) {
	inputCommand_ = inputCommand;
	targetWorld_ = targetWorld;
	playerWorld_ = playerWorld;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Camera");
	debugParame_->Register("PositionLerpRate", kPositionLerpRate_);
	debugParame_->Register("TargetLerpRate", kTargetLerpRate_);

	debugParame_->Register("OffsetY", offsetY_, 0, "Follow");
	debugParame_->Register("Distance", kDistance_, 0, "Follow");
	debugParame_->Register("RotateSpeed", kRotateSpeed_, 0, "Follow");
	debugParame_->Register("RotateDamping", kRotateDamping_, 0, "Follow");

	debugParame_->Register("MinLockOnDistance", kMinLockOnDistance_, 0, "LockOn");
	debugParame_->Register("LockOnPlayerOffsetY", lockOnPlayerOffsetY_, 0, "LockOn");
	debugParame_->Register("LockOnTargetOffsetY", lockOnTargetOffsetY_, 0, "LockOn");
}

void CameraController::Initialize() {
	// カメラの初期化
	camera_ = std::make_unique<Camera>();
	camera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},position_ }, 1280, 720);

	currentTarget_ = targetWorld_->GetWorldPosition();
	currentTarget_.y = 1.0f;
}

void CameraController::Update() {
	debugParame_->ApplyIfDirty();

	float dt60 = FpsCounter::deltaTime * FpsCounter::maxFrameCount;

	// 目標の位置と注視点
	Vector3 idealTarget = { 0.0f, 0.0f, 0.0f };
	Vector3 idealPosition = position_;

	if (inputCommand_->IsCommandActive("CameraLockOn")) {
		isLockOn_ = !isLockOn_;
	}

	if (isLockOn_) {
		Vector3 playerPos = playerWorld_->transform_.translate;
		Vector3 enemyPos = targetWorld_->transform_.translate;

		// プレイヤーと敵の中間点を注視点にする
		idealTarget = (playerPos + enemyPos) * 0.5f;
		idealTarget.y += lockOnPlayerOffsetY_;

		// プレイヤーから敵への水平方向のベクトルを計算
		Vector3 dir = enemyPos - playerPos;
		dir.y = 0.0f;

		// 水平距離を計測
		float dist = dir.Length();
		if (dist > 0.001f) {
			dir.x /= dist;
			dir.z /= dist;
		} else {
			// 完全に重なっている場合
			dir = { 0.0f, 0.0f, 1.0f };
		}

		// 距離に応じてカメラの引き具合を動的に設定
		float currentDistance = kMinLockOnDistance_ + dist * 0.6f;

		// カメラの位置を決定
		idealPosition.x = idealTarget.x + dir.x * currentDistance;
		idealPosition.z = idealTarget.z + dir.z * currentDistance;
		// カメラの高さを調整。距離が離れる程見下ろすような視点にする。
		idealPosition.y = idealTarget.y + lockOnTargetOffsetY_ + dist * 0.1f;

		// ロックオンを解除した時にカメラが急反転しないよう、現在の角度を球面座標系に同期しておく
		rotateMove_.x = std::atan2f(-dir.x, -dir.z);
		rotateVelocityX_ = 0.0f; // 慣性をリセット
	} else {
		idealTarget = targetWorld_->GetWorldPosition();
		idealTarget.y = offsetY_;

		// カメラ操作
		float targetRotateSpeed = 0.0f;
		if (inputCommand_->IsCommandActive("CameraMoveLeft")) {
			targetRotateSpeed += kRotateSpeed_;
		}

		if (inputCommand_->IsCommandActive("CameraMoveRight")) {
			targetRotateSpeed -= kRotateSpeed_;
		}

		// 目標の速度へ
		float currentDamping = std::powf(kRotateDamping_, dt60);
		rotateVelocityX_ = rotateVelocityX_ * currentDamping + targetRotateSpeed * (1.0f - currentDamping);
		rotateMove_.x += rotateVelocityX_ * FpsCounter::deltaTime;

		// 球面座標系で移動
		idealPosition.x = idealTarget.x + kDistance_ * std::sinf(rotateMove_.y) * std::sinf(rotateMove_.x);
		idealPosition.y = idealTarget.y + kDistance_ * std::cosf(rotateMove_.y);
		idealPosition.z = idealTarget.z + kDistance_ * std::sinf(rotateMove_.y) * std::cosf(rotateMove_.x);
	}

	// 補間
	float actualTargetLerp = 1.0f - std::pow(1.0f - kTargetLerpRate_, dt60);
	float actualPositionLerp = 1.0f - std::pow(1.0f - kPositionLerpRate_, dt60);
	currentTarget_ = Lerp(currentTarget_, idealTarget, actualTargetLerp);
	position_ = Lerp(position_, idealPosition, actualPositionLerp);

	// 回転行列に変換
	Matrix4x4 rotateMatrix_ = LookAt(position_, currentTarget_, { 0.0f,1.0f,0.0f });

	// ワールド行列
	Matrix4x4 worldMatrix_ = rotateMatrix_;
	worldMatrix_.m[3][0] = position_.x;
	worldMatrix_.m[3][1] = position_.y;
	worldMatrix_.m[3][2] = position_.z;

	// ワールド行列を設定
	camera_->SetWorldMatrix(worldMatrix_);
	// ワールド行列から更新する
	camera_->UpdateFromWorldMatrix();
}

Matrix4x4 CameraController::LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
	Vector3 f = Math::Normalize(center - eye); // 前方向ベクトル
	Vector3 s = Math::Normalize(Math::Cross(up, f)); // 右方向ベクトル
	Vector3 u = Math::Cross(f, s); // 上方向ベクトル

	Matrix4x4 result = { {
		{ s.x,  s.y, s.z, 0 },
		{ u.x,  u.y, u.z, 0 },
		{ f.x,  f.y, f.z, 0 },
		{ 0.0f, 0.0f, 0.0f, 1}
	} };
	return result;
}