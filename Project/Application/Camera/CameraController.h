#pragma once
#include "IGameObject.h"
#include "Camera.h"
#include "InputCommand.h"
#include "WorldTransform.h"
#include "DebugParameter.h"

class CameraController : public GameEngine::IGameObject {
public:
	CameraController(GameEngine::InputCommand* inputCommand, const GameEngine::WorldTransform* targetWorld, const GameEngine::WorldTransform* playerWorld);
	~CameraController() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	/// <summary>
	/// カメラデータ
	/// </summary>
	/// <returns></returns>
	GameEngine::Camera& GetCamera() const { return *camera_.get(); }

	Matrix4x4 GetWorldMatrix() const { return camera_->GetWorldMatrix(); }

	// ロックオン
	bool IsLockOn() const { return isLockOn_; }

private:
	// 距離
	float kDistance_ = 40.0f;
	float offsetY_ = 1.0f;

	float kMinLockOnDistance_ = 20.0f;
	float lockOnPlayerOffsetY_ = 1.0f;
	float lockOnTargetOffsetY_ = 6.0f;
	// 旋回速度
	float lockOnRotateRate_ = 0.06f;

	// 位置の追従
	float kPositionLerpRate_ = 0.08f;
	// 注視点の追従
	float kTargetLerpRate_ = 0.12f;

	// 回転の入力速度
	float kRotateSpeed_ = 2.0f;
	// 回転の減衰率
	float kRotateDamping_ = 0.88f;

	float kFollowRotateY_ = -1.2f;
	float kFollowFov_ = 0.45f;

	// 敵が近くにいる時のFov
	float kLockOnNearFov_ = 0.75f;
	// 敵から離れている時の通常Fov
	float kLockOnFarFov_ = 0.45f;

	// Fovが最大になる距離
	float kLockOnFovMinDist_ = 5.0f;
	// Fovが最小になる距離
	float kLockOnFovMaxDist_ = 35.0f;

	// Fovの補間
	float kFovLerpRate_ = 0.1f;

private:
	GameEngine::InputCommand* inputCommand_ = nullptr;
	const GameEngine::WorldTransform* targetWorld_ = nullptr;
	const GameEngine::WorldTransform* playerWorld_ = nullptr;

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// カメラ
	std::unique_ptr<GameEngine::Camera> camera_;
	Vector3 position_ = { 0.0f,4.0f,-10.0f };
	Vector3 currentTarget_ = { 0.0f,0.0f,0.0f };

	// 回転速度
	float rotateVelocityX_ = 0.0f;

	bool isLockOn_ = false;

	// 回転の移動量
	Vector2 rotateMove_ = { 3.1f,1.0f };

	float currentFov_ = 0.45f;

	float targetFov_ = 0.45f;

private:

	/// <summary>
	/// カメラをターゲットの方向に向かせる
	/// </summary>
	/// <param name="eye">カメラの位置</param>
	/// <param name="center">ターゲットの位置</param>
	/// <param name="up">向き</param>
	/// <returns></returns>
	Matrix4x4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up);
};