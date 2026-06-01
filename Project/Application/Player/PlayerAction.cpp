#define NOMINMAX
#include "PlayerAction.h"
#include <algorithm>
#include "InputCommand.h"
#include "MyMath.h"
#include "FpsCounter.h"
#include "DebugParameter.h"
#include "LogManager.h"
using namespace GameEngine;

//=====================================================
// プレイヤーが受ける物理
//=====================================================

void PlayerPhysics::Initialize(PlayerCommonData* commonData) {
	commonData_ = commonData;
}

void PlayerPhysics::Update() {
	// 重力を適応
	commonData_->velocity.y += kFallAcceleration_ * FpsCounter::deltaTime;
	commonData_->velocity.y = std::max(commonData_->velocity.y, -kMaxFallSpeed_);
}

//======================================================
// プレイヤーの移動アクション
//======================================================

void PlayerMoveAction::Initialize(PlayerCommonData* commonData, GameEngine::InputCommand* inputCommand) {
	commonData_ = commonData;
	inputCommand_ = inputCommand;
}

void PlayerMoveAction::ProcessMoveInput() {

	bool isJump = commonData_->state == PlayerState::kJump;

	Vector3 dir = { 0,0,0 };
	// XZの目標速度
	Vector3 desiredVelocityXZ = { 0,0,0 };

	// 移動の操作
	if (inputCommand_->IsCommandActive("MoveUp")) { dir -= cameraForwardXZ_; }
	if (inputCommand_->IsCommandActive("MoveDown")) { dir += cameraForwardXZ_; }
	if (inputCommand_->IsCommandActive("MoveLeft")) { dir -= cameraRightXZ_; }
	if (inputCommand_->IsCommandActive("MoveRight")) { dir += cameraRightXZ_; }

	if (dir.x != 0.0f || dir.z != 0.0f) {
		dir.y = 0.0f;
		dir.Normalize();
		
		// 最大移動速度を受け取る
		const float maxSpeed = isJump ? kAirMaxMoveSpeed_ : kGroundMaxMoveSpeed_;
		// 加速度を受け取る
		float acceleration = isJump ? kAirAcceleration_ : kGroundAcceleration_;

		// 現在の水平速度が既にmaxSpeed以上なら入力で増やさない
		Vector3 horiz = { commonData_->velocity.x, 0.0f, commonData_->velocity.z };
		float proj = horiz.x * dir.x + horiz.z * dir.z;
		float horizLen = Math::Length(horiz);
		if (horizLen >= maxSpeed) {
			desiredVelocityXZ = dir * proj;
		} else {
			desiredVelocityXZ += dir * acceleration;
		}
	}
	// 目標方向
	commonData_->targetDir = dir;
	
	// ノーマルとジャンプ状態以外は移動を無効化
	if (commonData_->state != PlayerState::kNone && commonData_->state != PlayerState::kJump) {
		desiredVelocityXZ = Vector3(0.0f, 0.0f, 0.0f);
	}

	// 加速,減速
	Vector3 target = { 0,0,0 };
	float deltaSpeed = 0.0f;
	if (desiredVelocityXZ.x == 0.0f && desiredVelocityXZ.z == 0.0f) {
		// 減速
		const float deceleration = isJump ? kAirDeceleration_ : kGroundDeceleration_;
		deltaSpeed = deceleration * FpsCounter::deltaTime;
	} else {
		// 加速
		const float acceleration = isJump ? kAirAcceleration_ : kGroundAcceleration_;
		deltaSpeed = acceleration * FpsCounter::deltaTime;
		target = desiredVelocityXZ;
	}

	// 適応
	commonData_->velocity.x = MoveTowards(commonData_->velocity.x, target.x, deltaSpeed);
	commonData_->velocity.z = MoveTowards(commonData_->velocity.z, target.z, deltaSpeed);
}

void PlayerMoveAction::UpdateCameraBasis(const Matrix4x4& cameraWorldMatrix) {
	// カメラからのZ軸
	Vector3 forward = {
	-cameraWorldMatrix.m[2][0],
	-cameraWorldMatrix.m[2][1],
	-cameraWorldMatrix.m[2][2]
	};
	forward.y = 0.0f;
	if (forward.x != 0.0f || forward.z != 0.0f) {
		forward.Normalize();
	}

	// カメラからのX軸
	Vector3 right = {
		cameraWorldMatrix.m[0][0],
		cameraWorldMatrix.m[0][1],
		cameraWorldMatrix.m[0][2]
	};
	right.y = 0.0f;
	if (right.x != 0.0f || right.z != 0.0f) {
		right.Normalize();
	}

	cameraForwardXZ_ = forward;
	cameraRightXZ_ = right;
}

float PlayerMoveAction::MoveTowards(float current, float target, float maxDelta) {
	const float diff = target - current;

	if (std::fabs(diff) <= maxDelta) {
		return target;
	}

	return current + (diff > 0.0f ? maxDelta : -maxDelta);
}

void PlayerMoveAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "Move";
	int index = 0;
	param->Register("GroundMaxMoveSpeed", kGroundMaxMoveSpeed_, index++, subGroup);
	param->Register("AirMaxMoveSpeed", kAirMaxMoveSpeed_, index++, subGroup);
	param->Register("GroundAcceleration", kGroundAcceleration_, index++, subGroup);
	param->Register("AirAcceleration", kAirAcceleration_, index++, subGroup);
	param->Register("GroundDeceleration", kGroundDeceleration_, index++, subGroup);
	param->Register("AirDeceleration", kAirDeceleration_, index++, subGroup);
}

//=======================================================
// プレイヤーの突進アクション
//=======================================================

void PlayerAttackRushAction::Initialize(PlayerCommonData* commonData, GameEngine::InputCommand* inputCommand) {
	commonData_ = commonData;
	inputCommand_ = inputCommand;
}

void PlayerAttackRushAction::ProcessInput() {
	// ため状態
	if (inputCommand_->IsCommandActive("RushCharge")) {
		if (commonData_->state == PlayerState::kNone) {
			chargeTimer_ = 0.0f;
			chargeRatio_ = 0.0f;
			// Rush方向初期化
			Vector3 rushDirXZ = commonData_->currentDir;
			if (rushDirXZ.x == 0.0f && rushDirXZ.z == 0.0f) { rushDirXZ = { commonData_->velocity.x, 0.0f, commonData_->velocity.z }; }
			rushDirection_ = (rushDirXZ.x == 0.0f && rushDirXZ.z == 0.0f) ? commonData_->cameraForwardXZ : rushDirXZ.Normalize();
			// 溜め開始時はレベルをリセット
			rushChargeLevel_ = 0;
			// 現在の状態
			commonData_->state = PlayerState::kCharging;
			Log("Player start charge");
		}	
	}

	// 発射状態
	if (inputCommand_->IsCommandActive("RushStart")) {
		if (commonData_->state == PlayerState::kCharging) {
			// 予備動作時間を溜め比率で決定
			chargeRatio_ = std::clamp(chargeTimer_, 0.0f, 1.0f);
			// 溜め比率に応じてレベル決定
			if (chargeRatio_ < kRushChargeLevel2Ratio_) {
				rushChargeLevel_ = 1;
			} else if (chargeRatio_ < kRushChargeLevel3Ratio_) {
				rushChargeLevel_ = 2;
			} else {
				rushChargeLevel_ = 3;
			}
			rushTimer_ = 0.0f;

			float levelMultiplier = 1.0f;
			switch (rushChargeLevel_) {
			case 1: levelMultiplier = kRushStrengthLevel1_; break;
			case 2: levelMultiplier = kRushStrengthLevel2_; break;
			case 3: levelMultiplier = kRushStrengthLevel3_; break;
			default: levelMultiplier = kRushStrengthLevel1_; break;
			}
			float rushSpeed = kRushMaxSpeed_ * levelMultiplier;
			Vector3 initVel = rushDirection_ * rushSpeed;
			commonData_->velocity.x = initVel.x;
			commonData_->velocity.z = initVel.z;
			// 現在の状態
			commonData_->state = PlayerState::kAttackRush;
			Log("Player start attackRush");
		}
	}
}

void PlayerAttackRushAction::Update() {

	// ため時間計測
	if (commonData_->state == PlayerState::kCharging) {
		chargeTimer_ += FpsCounter::deltaTime / kRushChargeMaxTime_;
	}

	// 突進
	if (commonData_->state == PlayerState::kAttackRush) {
		rushTimer_ += FpsCounter::deltaTime / kRushMaxTime_;

		if (rushTimer_ >= 1.0f) {
			Log("Player end attackRush");
			commonData_->state = PlayerState::kNone;
		}
	}

	// 突進硬直のクールタイム
	if (commonData_->state == PlayerState::kStiffness) {
		coolTime_ += FpsCounter::deltaTime / kRushCooldownTime_;

		if (coolTime_ >= 1.0f) {
			commonData_->state = PlayerState::kNone;
		}
	}

}

void PlayerAttackRushAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "AttackRush";
	int index = 0;
	param->Register("PreRushMaxTime", kPreRushMaxTime_, index++, subGroup);
	param->Register("RushMaxSpeed", kRushMaxSpeed_, index++, subGroup);
	param->Register("RushLockMaxTime", kRushLockMaxTime_, index++, subGroup);
	param->Register("RushChargeMaxTime", kRushChargeMaxTime_, index++, subGroup);

	param->Register("RushChargeLevel1Ratio", kRushChargeLevel1Ratio_, index++, subGroup);
	param->Register("RushChargeLevel2Ratio", kRushChargeLevel2Ratio_, index++, subGroup);
	param->Register("RushChargeLevel3Ratio", kRushChargeLevel3Ratio_, index++, subGroup);

	param->Register("RushCooldownTime", kRushCooldownTime_, index++, subGroup);

	param->Register("RushStrengthLevel1", kRushStrengthLevel1_, index++, subGroup);
	param->Register("RushStrengthLevel2", kRushStrengthLevel2_, index++, subGroup);
	param->Register("RushStrengthLevel3", kRushStrengthLevel3_, index++, subGroup);

	param->Register("RushMaxTime", kRushMaxTime_, index++, subGroup);
}

//=======================================================
// プレイヤーの跳ね返りアクション
//=======================================================

void PlayerBounceAction::Initialize(PlayerCommonData* commonData) {
	commonData_ = commonData;
}

void PlayerBounceAction::WallBounce(Vector3& pos,const Vector3& bounceDirection, const float& penetrationDepth) {

	// 壁に衝突していたら押し戻す
	Vector3 dirXZ = { bounceDirection.x, 0.0f, bounceDirection.z };
	if (dirXZ.x != 0.0f || dirXZ.z != 0.0f) { dirXZ.Normalize(); }
	float depth = std::max(penetrationDepth, 0.0f);
	Vector3 correction = { dirXZ.x * depth, 0.0f, dirXZ.z * depth };
	pos.x += correction.x;
	pos.z += correction.z;

	// 速度と方向を変更する
	Vector3 velocityXZ = { commonData_->velocity.x, 0.0f, commonData_->velocity.z };
	float dot = velocityXZ.x * dirXZ.x + velocityXZ.z * dirXZ.z;
	if (dot < 0.0f) {
		Vector3 reflected = {
			velocityXZ.x - 2.0f * dot * dirXZ.x,
			0.0f,
			velocityXZ.z - 2.0f * dot * dirXZ.z
		};
		commonData_->velocity.x = reflected.x * kWallBounceReflectFactor_;
		commonData_->velocity.z = reflected.z * kWallBounceReflectFactor_;
		Vector3 newDir = { reflected.x, 0.0f, reflected.z };
		float len = Math::Length(newDir);
		if (len > 0.00001f) {
			commonData_->targetDir = Math::Normalize(newDir);
		}
	}

	// ラッシュ状態からの突進であれば上に飛ぶ
	if (commonData_->state == PlayerState::kAttackRush) {
		float levelMultiplier = 1.0f;
		switch (rushChargeLevel_) {
		case 1: levelMultiplier = kWallBounceStrengthLevel1_; break;
		case 2: levelMultiplier = kWallBounceStrengthLevel2_; break;
		case 3: levelMultiplier = kWallBounceStrengthLevel3_; break;
		default: levelMultiplier = kWallBounceStrengthLevel1_; break;
		}
		commonData_->velocity.y = kWallBounceUpSpeed_ * kWallBounceReflectFactor_ * levelMultiplier;
		//currentBounceLockTime_ = kWallBounceLockTime_;
	}
}

void PlayerBounceAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "Bounce";
	int index = 0;
	param->Register("WallBounceReflectFactor", kWallBounceReflectFactor_, index++, subGroup);
}

//=======================================================
// プレイヤーの急降下攻撃アクション
//=======================================================

void PlayerAttackDownAction::Initialize(PlayerCommonData* commonData) {
	commonData_ = commonData;
}

void PlayerAttackDownAction::Update() {
		


}

void PlayerAttackDownAction::RegisterParameter(GameEngine::DebugParameter* param) {



}