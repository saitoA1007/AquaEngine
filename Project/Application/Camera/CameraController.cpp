#include "pch.h"
#define NOMINMAX
#include <algorithm>
#include "CameraController.h"
#include "MyMath.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "State/CameraState.h"
using namespace GameEngine;

CameraController::CameraController(GameEngine::InputCommand* inputCommand, const GameEngine::WorldTransform* targetWorld, const GameEngine::WorldTransform* playerWorld) {
	inputCommand_ = inputCommand;
	targetWorld_ = targetWorld;
	playerWorld_ = playerWorld;

	// カメラの初期化
	camera_ = std::make_unique<Camera>();
	camera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},position_ }, 1280, 720);

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Camera");
	
	// 登録
	states_[CameraState::kFollow] = std::make_unique<FollowCameraState>(this, debugParame_.get());
	states_[CameraState::kLockOn] = std::make_unique<LockOnCameraState>(this, debugParame_.get());
	states_[CameraState::kTitle] = std::make_unique<TitleCameraState>(this, debugParame_.get());
	states_[CameraState::kEnterMovie] = std::make_unique<EnterMovieCameraState>(this, debugParame_.get());
	states_[CameraState::kClearMovie] = std::make_unique<ClearMovieCameraState>(this, debugParame_.get());

	// パラメーターの値を適応
	debugParame_->Apply();

	currentStateType_ = CameraState::kFollow;
	currentState_ = states_[currentStateType_].get();
	currentState_->Enter();
}

void CameraController::Initialize() {
	// カメラの位置を初期化
	camera_->transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,4.0f,-10.0f } };

	currentTarget_ = targetWorld_->GetWorldPosition();
	currentTarget_.y = 1.0f;

	// 揺れを停止
	shakeTimer_ = 1.0f;

	currentStateType_ = CameraState::kFollow;
	currentState_ = states_[currentStateType_].get();
	currentState_->Enter();
}

void CameraController::Update() {
	debugParame_->ApplyIfDirty();
	
	float dt60 = FpsCounter::deltaTime * FpsCounter::maxFrameCount;
	
	// カメラを切り替える
	CameraCommonData& common = currentState_->GetCommonData();

	// 内部からの切り替え
	if (common.requestState.has_value()) {
		ChangeState(common.requestState.value());
		common.requestState = std::nullopt;
	}
	// 外部からの切り替え
	if (requestState.has_value()) {
		ChangeState(requestState.value());
		requestState = std::nullopt;
	}

	// 更新
	currentState_->Update(dt60);
	
	// 補間
	float actualTargetLerp = 1.0f - std::powf(1.0f - common.targetLerpRate, dt60);
	float actualPositionLerp = 1.0f - std::powf(1.0f - common.positionLerpRate, dt60);
	currentTarget_ = Lerp(currentTarget_, common.idealTarget, actualTargetLerp);

	// 補間前の注視点との距離
	float prevDistance = (position_ - common.idealTarget).Length();
	position_ = Lerp(position_, common.idealPosition, actualPositionLerp);

	if (common.keepDistance) {
		// 直線で補間すると回り込む時に内側を通って近づくため、距離は別で補間して保つ
		float idealDistance = (common.idealPosition - common.idealTarget).Length();
		float distance = prevDistance + (idealDistance - prevDistance) * actualPositionLerp;

		Vector3 toCamera = position_ - common.idealTarget;
		float length = toCamera.Length();
		if (length > 0.0001f) {
			position_ = common.idealTarget + toCamera * (distance / length);
		}
	}
	// Fovを補間
	targetFov_ = common.targetFov;
	float actualFovLerp = 1.0f - std::powf(1.0f - common.fovLerpRate, dt60);
	currentFov_ = currentFov_ + (targetFov_ - currentFov_) * actualFovLerp;
	camera_->SetProjectionMatrix(currentFov_, 1280, 720, 0.1f, 200.0f);
	
	// 揺れを加える
	Vector3 shakeOffset = UpdateShake();
	Vector3 eye = position_ + shakeOffset;
	Vector3 target = currentTarget_ + shakeOffset;

	// 回転行列に変換
	Matrix4x4 rotateMatrix_ = LookAt(eye, target, { 0.0f,1.0f,0.0f });
	// ワールド行列
	Matrix4x4 worldMatrix_ = rotateMatrix_;
	worldMatrix_.m[3][0] = eye.x;
	worldMatrix_.m[3][1] = eye.y;
	worldMatrix_.m[3][2] = eye.z;
	
	// ワールド行を設定
	camera_->SetWorldMatrix(worldMatrix_);
	camera_->UpdateFromWorldMatrix();
}

void CameraController::ChangeState(CameraState next) {
	currentState_->Exit();
	currentStateType_ = next;
	currentState_ = states_[currentStateType_].get();
	currentState_->Enter();
}

void CameraController::StartShake(float intensity, float duration) {
	// 揺れ中であれば強い方を優先する
	float t = std::clamp(shakeTimer_, 0.0f, 1.0f);
	float currentIntensity = shakeIntensity_ * (1.0f - t);
	if (intensity < currentIntensity) { return; }

	shakeIntensity_ = intensity;
	shakeDuration_ = duration;
	shakeTimer_ = 0.0f;
}

Vector3 CameraController::UpdateShake() {
	if (shakeTimer_ >= 1.0f || shakeDuration_ <= 0.0f) { return { 0.0f,0.0f,0.0f }; }

	shakeTimer_ += FpsCounter::deltaTime / shakeDuration_;
	float t = std::clamp(shakeTimer_, 0.0f, 1.0f);
	// 時間経過で減衰
	float power = shakeIntensity_ * (1.0f - t) * (1.0f - t);

	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
	return Vector3(dist(randomEngine_), dist(randomEngine_), dist(randomEngine_)) * power;
}

Matrix4x4 CameraController::LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
	Vector3 f = Vector3::Normalize(center - eye); // 前方向ベクトル
	Vector3 s = Vector3::Normalize(up.Cross(f)); // 右方向ベクトル
	Vector3 u = f.Cross(s); // 上方向ベクトル

	Matrix4x4 result = { {
		{ s.x,  s.y, s.z, 0 },
		{ u.x,  u.y, u.z, 0 },
		{ f.x,  f.y, f.z, 0 },
		{ 0.0f, 0.0f, 0.0f, 1}
	} };
	return result;
}