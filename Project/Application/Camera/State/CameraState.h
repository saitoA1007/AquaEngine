#pragma once
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

	float kFollowRotateY_ = -1.2f;
	float kFollowFov_ = 0.45f;

private:
	// フォロー固有の回転慣性
	float rotateVelocityX_ = 0.0f; 
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
	EnterMovieCameraState(CameraController* controller, GameEngine::DebugParameter* param);

	void Enter() override;
	void Update(float dt60) override;
	
private:
	
private:

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