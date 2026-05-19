#define NOMINMAX
#include "PlayerAction.h"
#include <algorithm>
#include "InputCommand.h"
#include "MyMath.h"
#include "FpsCounter.h"
#include "DebugParameter.h"
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
	Vector3 desiredVelXZ = { 0,0,0 };

	// 移動の操作
	if (inputCommand_->IsCommandAcitve("MoveUp")) { dir -= cameraForwardXZ_; }
	if (inputCommand_->IsCommandAcitve("MoveDown")) { dir += cameraForwardXZ_; }
	if (inputCommand_->IsCommandAcitve("MoveLeft")) { dir -= cameraRightXZ_; }
	if (inputCommand_->IsCommandAcitve("MoveRight")) { dir += cameraRightXZ_; }

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
			desiredVelXZ = dir * proj;
		} else {
			desiredVelXZ += dir * acceleration;
		}
	}
	// 目標方向
	commonData_->targetDir = dir;
	
	// 加速,減速を適応
	ApplyAxis(commonData_->velocity.x, desiredVelXZ.x, isJump);
	ApplyAxis(commonData_->velocity.z, desiredVelXZ.z, isJump);
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

void PlayerMoveAction::ApplyAxis(float& vel, float target, bool isAir) {
	const float acceleration = isAir ? kAirAcceleration_ : kGroundAcceleration_;
	const float deceleration = isAir ? kAirDeceleration_ : kGroundDeceleration_;

	// 目標方向へ加速
	if (target != 0.0f) {
		const float diff = target - vel;
		const float step = acceleration * FpsCounter::deltaTime;
		if (std::fabsf(diff) <= step) {
			vel = target;
		} else {
			vel += (diff > 0.0f ? step : -step);
		}
	} else {
		const float speed = std::fabs(vel);
		const float step = deceleration * FpsCounter::deltaTime;
		if (speed <= step) {
			vel = 0.0f;
		} else {
			vel += (vel > 0.0f ? -step : step);
		}
	}
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

void PlayerAttackRushAction::Initialize(PlayerCommonData* commonData) {
	commonData_ = commonData;
}

//=======================================================
// プレイヤーの跳ね返りアクション
//=======================================================

void PlayerBounceAction::Initialize(PlayerCommonData* commonData) {
	commonData_ = commonData;
}

void PlayerBounceAction::WallBounce(Vector3& pos,const Vector3& bounceDirection, const float& penetrationDepth) {

	// 壁に衝突していたら押し戻す
	Vector3 nXZ = { bounceDirection.x, 0.0f, bounceDirection.z };
	if (nXZ.x != 0.0f || nXZ.z != 0.0f) { nXZ.Normalize(); }
	float depth = std::max(penetrationDepth, 0.0f);
	Vector3 correction = { nXZ.x * depth, 0.0f, nXZ.z * depth };
	pos.x += correction.x;
	pos.z += correction.z;

	// 速度と方向を変更する
	Vector3 velXZ = { commonData_->velocity.x, 0.0f, commonData_->velocity.z };
	float dot = velXZ.x * nXZ.x + velXZ.z * nXZ.z;
	if (dot < 0.0f) {
		Vector3 reflected = {
			velXZ.x - 2.0f * dot * nXZ.x,
			0.0f,
			velXZ.z - 2.0f * dot * nXZ.z
		};
		commonData_->velocity.x = reflected.x * kWallBounceReflectFactor_;
		commonData_->velocity.z = reflected.z * kWallBounceReflectFactor_;
		Vector3 newDir = { reflected.x, 0.0f, reflected.z };
		float len = Math::Length(newDir);
		if (len > 0.00001f) {
			commonData_->targetDir = Math::Normalize(newDir);
		}
	}

}

//=======================================================
// プレイヤーの急降下攻撃アクション
//=======================================================

void PlayerAttackDownAction::Initialize(PlayerCommonData* commonData) {
	commonData_ = commonData;
}