#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"

namespace GameEngine {
	// 前方宣言
	class InputCommand;
	class DebugParameter;
}

// プレイヤーの状態
enum class PlayerState {
	kNone,
	kJump,
	kAttackRush,
	kCharging,
	kAttackDown,
	kBounce,
	kStiffness,

	kMaxCount
};

// プレイヤーの共通データ
struct PlayerCommonData {
	Vector3 velocity = {0.0f,0.0f,0.0f};
	// 現在向いている方向
	Vector3 currentDir = {0.0f,0.0f,1.0f};
	// 最終的に向く方向
	Vector3 targetDir = { 0.0f, 0.0f, 1.0f };
	// 現在の方向
	float currentYaw = 0.0f;

	// プレイヤーの状態
	PlayerState state = PlayerState::kNone;

	Vector3 cameraForwardXZ = { 0.0f,0.0f,1.0f };
	Vector3 cameraRightXZ = { 1.0f,0.0f,0.0f };
};

// プレイヤーアクションの基底クラス
class IPlayerAction {
public:
	virtual ~IPlayerAction() = default;

	// 値を登録する
	virtual void RegisterParameter([[maybe_unused]]GameEngine::DebugParameter* param) {};

protected:
	// プレイヤーの共通状態
	PlayerCommonData* commonData_ = nullptr;
};

// プレイヤーが常に受ける物理
class PlayerPhysics : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData);

	void Update();

	// パラメータを登録する
	//void RegisterParameter(GameEngine::DebugParameter* param) override;

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

	// パラメータを登録する
	void RegisterParameter(GameEngine::DebugParameter* param) override;

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
	
	float MoveTowards(float current, float target, float maxDelta);
};

// 突進アクション
class PlayerAttackRushAction : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData, GameEngine::InputCommand* inputCommand);

	void ProcessInput();

	void Update();

	// パラメータを登録する
	void RegisterParameter(GameEngine::DebugParameter* param) override;

private:
	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;

private:
	// 突撃予備動作最大時間
	float kPreRushMaxTime_ = 0.5f;
	// 突撃最大速度
	float kRushMaxSpeed_ = 32.0f;
	// 突進時硬直最大時間
	float kRushLockMaxTime_ = 1.0f;
	// 突進溜め最大時間
	float kRushChargeMaxTime_ = 2.0f;
	// 突進の強さLv1になるまでの時間の割合
	float kRushChargeLevel1Ratio_ = 0.0f;
	// 突進の強さLv2になるまでの時間の割合
	float kRushChargeLevel2Ratio_ = 0.5f;
	// 突進の強さLv3になるまでの時間の割合
	float kRushChargeLevel3Ratio_ = 1.0f;
	// 突進時クールタイム（秒。硬直終了後に次の突進が可能になるまでの時間）
	float kRushCooldownTime_ = 0.5f;

	// 突撃の強さ
	float kRushStrengthLevel1_ = 0.5f;
	float kRushStrengthLevel2_ = 0.8f;
	float kRushStrengthLevel3_ = 1.0f;

	// 突進している時の時間
	float kRushMaxTime_ = 2.0f;

private:
	float chargeTimer_ = 0.0f;
	float chargeRatio_ = 0.0f;
	Vector3 rushDirection_;
	uint32_t rushChargeLevel_ = 0;

	float rushTimer_ = 0.0f;
	float coolTime_ = 0.0f;
};

// 跳ね返りアクション
class PlayerBounceAction : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData);

	void WallBounce(Vector3& pos,const Vector3& bounceDirection,const float& penetrationDepth);

	// パラメータを登録する
	void RegisterParameter(GameEngine::DebugParameter* param) override;

private:
	// 跳ね上がり後の高さ
	float kWallBounceUpSpeed_ = 10.0f;
	// 跳ね上がり後の壁から離れる距離
	float kWallBounceAwaySpeed_ = 5.0f;
	// 跳ね返り直後の硬直時間
	float kWallBounceLockTime_ = 0.8f;

	// 速さに応じた跳ね返りの倍率の最低値
	float kWallBounceMinSpeedFactor_ = 0.5f;
	// 速さに応じた跳ね返りの倍率の最大値
	float kWallBounceMaxSpeedFactor_ = 1.5f;

	// 跳ね返る大きさ
	float kWallBounceStrengthLevel1_ = 0.5f;
	float kWallBounceStrengthLevel2_ = 0.75f;
	float kWallBounceStrengthLevel3_ = 1.0f;

	// 壁に衝突した際の跳ね返りの倍率
	float kWallBounceReflectFactor_ = 1.0f;

private:

	uint32_t rushChargeLevel_ = 3;

};

// 急降下攻撃アクション
class PlayerAttackDownAction : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData);

	void Update();

	// パラメータを登録する
	void RegisterParameter(GameEngine::DebugParameter* param) override;

private:


};


