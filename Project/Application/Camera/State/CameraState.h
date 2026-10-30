#pragma once
#include <numbers>
#include "Application/Camera/ICameraState.h"

// 前方宣言
namespace GameEngine {
	class DebugParameter;
}

// 追跡カメラ
class FollowCameraState : public ICameraState {
public:
	FollowCameraState(CameraController* controller, GameEngine::DebugParameter* param);

	void Enter() override;
	void Update(float dt60) override;

private:
	// 距離
	float kDistance_ = 40.0f;
	float offsetY_ = 1.0f;

	// 回転の入力速度
	float kRotateSpeed_ = 2.0f;
	// 回転の減衰率
	float kRotateDamping_ = 0.88f;

	// Y軸回転の初期値
	float kFollowRotateY_ = -1.2f;
	float kFollowFov_ = 0.45f;

	// Y軸回転の入力速度
	float kRotateSpeedY_ = 1.5f;
	// Y軸回転の範囲
	float kMinRotateY_ = -std::numbers::pi_v<float> * 0.5f;
	float kMaxRotateY_ = -0.01f;

	// 入力し続けた時の最大速度の倍率
	float kRotateMaxSpeedRate_ = 3.0f;
	// 最大速度に達するまでの入力時間
	float kRotateAccelTime_ = 1.0f;

private:
	// 入力し続けている時間と向き
	struct HoldInput {
		float time = 0.0f;
		float dir = 0.0f;
	};

	// フォロー固有の回転慣性
	float rotateVelocityX_ = 0.0f;
	float rotateVelocityY_ = 0.0f;

	// 各軸の入力状態
	HoldInput holdX_;
	HoldInput holdY_;

private:
	// 入力し続けた時間に応じた目標回転速度を求める
	float CalcHoldRotateSpeed(float inputDir, float baseSpeed, HoldInput& hold);
};

// ロックオンカメラ
class LockOnCameraState : public ICameraState {
public:
	LockOnCameraState(CameraController* controller, GameEngine::DebugParameter* param);

	void Update(float dt60) override;

private:
	// 敵が近くにいる時のFov
	float kLockOnNearFov_ = 0.75f;
	// 敵から離れている時の通常Fov
	float kLockOnFarFov_ = 0.45f;

	// Fovが最大になる距離
	float kLockOnFovMinDist_ = 5.0f;
	// Fovが最小になる距離
	float kLockOnFovMaxDist_ = 35.0f;

	float kMinLockOnDistance_ = 20.0f;
	float lockOnPlayerOffsetY_ = 1.0f;
	float lockOnTargetOffsetY_ = 6.0f;
	// 旋回速度
	float lockOnRotateRate_ = 0.06f;
};

// 入りのムービーカメラ
class EnterMovieCameraState : public ICameraState {
public:
	// カメラの状態
	enum class Phase {
		kMove,
		kWait,
	};

public:
	EnterMovieCameraState(CameraController* controller, GameEngine::DebugParameter* param);

	void Enter() override;

	void Update(float dt60) override;
	
private:

	// カメラの動く時間
	float kMoveMaxTime_ = 2.0f;
	// カメラの待機時間
	float kWaitMaxTime_ = 1.0f;

	Vector3 kMovePos_ = { 0.0f,0.0f,0.0f };
	float kMoveOfferY_ = 0.0f;
	float kMoveFov_ = 0.9f;

	Vector3 kWaitPos_ = { 0.0f,0.0f,0.0f };
	float kWaitFov_ = 0.7f;
private:

	float timer_ = 0.0f;

	Phase phase_ = Phase::kMove;
};

// クリアのムービーカメラ
class ClearMovieCameraState : public ICameraState {
public:
	ClearMovieCameraState(CameraController* controller, GameEngine::DebugParameter* param);

	void Enter() override;
	void Update(float dt60) override;

private:

private:

};

// タイトルカメラ
class TitleCameraState : public ICameraState {
public:
	TitleCameraState(CameraController* controller, GameEngine::DebugParameter* param);

	void Enter() override;
	void Update(float dt60) override;

};