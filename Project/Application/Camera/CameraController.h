#pragma once
#include "IGameObject.h"
#include "Camera.h"
#include "InputCommand.h"
#include "WorldTransform.h"
#include "DebugParameter.h"
#include "ICameraState.h"

class CameraController : public GameEngine::IGameObject {
public:
	CameraController(GameEngine::InputCommand* inputCommand, const GameEngine::WorldTransform* targetWorld, const GameEngine::WorldTransform* playerWorld);
	~CameraController() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

public:

	/// <summary>
	/// カメラデータ
	/// </summary>
	/// <returns></returns>
	GameEngine::Camera& GetCamera() const { return *camera_.get(); }

	Matrix4x4 GetWorldMatrix() const { return camera_->GetWorldMatrix(); }

	GameEngine::InputCommand* GetInputCommand() const { return inputCommand_; }
	const GameEngine::WorldTransform* GetTargetWorld() const { return targetWorld_; }
	const GameEngine::WorldTransform* GetPlayerWorld() const { return playerWorld_; }

	Vector2& GetRotateMove() { return rotateMove_; }

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

	// 回転の移動量
	Vector2 rotateMove_ = { 3.1f,1.0f };

	float currentFov_ = 0.45f;

	float targetFov_ = 0.45f;

	std::array<std::unique_ptr<ICameraState>, static_cast<size_t>(CameraState::kMaxCount)> states_;
	ICameraState* currentState_ = nullptr;
	CameraState currentStateType_ = CameraState::kFollow;
private:

	// カメラの状態を切り替える
	void ChangeState(CameraState next);

	/// <summary>
	/// カメラをターゲットの方向に向かせる
	/// </summary>
	/// <param name="eye">カメラの位置</param>
	/// <param name="center">ターゲットの位置</param>
	/// <param name="up">向き</param>
	/// <returns></returns>
	Matrix4x4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up);
};