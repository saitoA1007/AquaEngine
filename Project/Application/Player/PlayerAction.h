#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"

namespace GameEngine {
	// 前方宣言
	class InputCommand;
}

// プレイヤーの状態
enum class PlayerState {
	kNone,
	kJump,
	kAttackRush,
	kCharging,
	kAttackDown,
	kBounce,

	kMaxCount
};

// プレイヤーの共通データ
struct PlayerCommonData {
	Vector3 velocity;
	// 現在向いている方向
	Vector3 currentDir;
	// 最終的に向く方向
	Vector3 targetDir = { 0.0f, 0.0f, 1.0f };
	// 現在の方向
	float currentYaw = 0.0f;

	// プレイヤーの状態
	PlayerState state;
};

// プレイヤーアクションの基底クラス
class IPlayerAction {
public:
	virtual ~IPlayerAction() = default;

protected:
	// プレイヤーの共通状態
	PlayerCommonData* commonData_ = nullptr;
};

// プレイヤーが常に受ける物理
class PlayerPhysics : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData);

	void Update();

private:
	// 落下速度の上限
	float kMaxFallSpeed_ = 2.0f;
	// 落下加減速量
	float kFallAcceleration_ = -9.6f;
};

// 移動アクション
class PlayerMoveAction : public IPlayerAction {
public:
	// 初期化
	void Initialize(PlayerCommonData* commonData, GameEngine::InputCommand* inputCommand);

	// 入力
	void ProcessMoveInput();

	// カメラ基準のベクトルを更新する
	void UpdateCameraBasis(const Matrix4x4& cameraWorldMatrix);

private:
	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;

private:
	// 地上の移動速度
	float kGroundMaxMoveSpeed_ = 16.0f;
	// 空中の移動速度
	float kAirMaxMoveSpeed_ = 16.0f;
	// 地上での加速量
	float kGroundAcceleration_ = 8.0f;
	// 空中での加速量
	float kAirAcceleration_ = 2.0f;
	// 地上での減速量
	float kGroundDeceleration_ = 2.0f;
	// 空中での減速量
	float kAirDeceleration_ = 1.0f;
private:

	// カメラ基準
	Vector3 cameraForwardXZ_ = { 0.0f,0.0f,1.0f };
	Vector3 cameraRightXZ_ = { 1.0f,0.0f,0.0f };

private:
	void ApplyAxis(float& vel, float target, bool isAir);
};

// 突進アクション
class PlayerAttackRushAction : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData);

};

// 跳ね返りアクション
class PlayerBounceAction : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData);


};

// 急降下攻撃アクション
class PlayerAttackDownAction : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData);

};


