#define NOMINMAX
#include "BossBattleAction.h"
#include <numbers>
#include "FPSCounter.h"
#include "MyMath.h"
#include "EasingManager.h"
#include "RandomGenerator.h"
#include "Application/Enemy/BossAnimator.h"
using namespace GameEngine;

//=============================================================
// 突進攻撃
//=============================================================

BossRushAttackAction::BossRushAttackAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void BossRushAttackAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;
	state_ = State::kMove;

	Vector3 myDir = commonData_.transform.translate;
	myDir.y = 0.0f;
	myDir.Normalize();
	// 最初の角度
	startAngle_ = std::atan2f(myDir.z, myDir.x);

	Vector3 playerDir = *commonData_.playerPos;
	playerDir.y = 0.0f;
	playerDir.Normalize();
	playerDir = playerDir * -1.0f;
	float tmpEndAngle = std::atan2f(playerDir.z, playerDir.x);
	float diffAngle = Math::GetAngleDiff(startAngle_, tmpEndAngle);
	// 最後の角度
	endAngle_ = startAngle_ + diffAngle;

	// 突進の位置を取得
	startRushPos_ = playerDir * commonData_.stageRadius;
	startRushPos_.y = defaultPosY_;
	endRushPos_ = myDir * commonData_.stageRadius;
	endRushPos_.y = defaultPosY_;

	commonData_.animator->StartAnimation(BossAnimationType::kRush, "Rush_Prepare", rotateMoveMaxTime_,false);
}

void BossRushAttackAction::Update() {

	switch (state_)
	{
	case BossRushAttackAction::State::kMove:
		RotateMove();
		break;


	case BossRushAttackAction::State::kRush:
		RushAttack();
		break;
	}
}

void BossRushAttackAction::Finalize() {

}

void BossRushAttackAction::RotateMove() {
	timer_ += FpsCounter::deltaTime / rotateMoveMaxTime_;

	// 角度補間
	float preAngle = angle_;
	angle_ = Lerp(startAngle_, endAngle_, timer_);

	float r = 0.0f;
	if (timer_ >= 0.4f && timer_ < 0.8f) {
		float localT = (timer_ - 0.4f) / 0.4f;
		r = Lerp(0.0f, 2.0f, localT);
	} else if (timer_ >= 0.8f) {
		float localT = (timer_ - 0.8f) / 0.2f;
		r = Lerp(2.0f, 0.0f, localT);
	}

	// 膨らむような円の軌跡
	float radius = commonData_.stageRadius + r;

	// 回転移動
	commonData_.transform.translate = GetXZFromAngle(angle_, radius, defaultPosY_);
	
	// 回転
	commonData_.transform.rotate.z = 0.2f;
	Vector3 prePos = GetXZFromAngle(preAngle, radius, defaultPosY_);
	Vector3 dir = commonData_.transform.translate - prePos;
	dir.Normalize();
	commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

	if (timer_ >= 1.0f) {
		state_ = State::kRush;
		timer_ = 0.0f;

		// プレイヤーの位置から突進する位置を求める
		Vector3 targetDir = *commonData_.playerPos - commonData_.transform.translate;
		targetDir.y = 0.0f;
		targetDir.Normalize();

		// ボスのステージの中心方向へのベクトル
		Vector3 myDir = commonData_.transform.translate * -1.0f;
		myDir.y = 0.0f;
		myDir.Normalize();

		float myAngle = std::atan2f(myDir.z, myDir.x);
		float targetAngle = std::atan2f(targetDir.z, targetDir.x);
		// 現在の方向と突進する方向の差を求める
		float diffAngle = Math::GetAngleDiff(myAngle, targetAngle);

		// 角度差を範囲に制限する
		float limitAngle = std::numbers::pi_v<float> / 6.0f;
		if (diffAngle > limitAngle) {
			diffAngle = limitAngle;
		} else if(diffAngle < -limitAngle) {
			diffAngle = -limitAngle;
		}

		// 突進する方向を求める
		float rushAngle = myAngle + diffAngle;
		targetDir = Vector3(std::cosf(rushAngle), 0.0f, std::sinf(rushAngle));

		// 突進する位置を求める
		float rushDistance = commonData_.stageRadius * 2.0f * std::cosf(diffAngle);
		endRushPos_ = commonData_.transform.translate + targetDir * rushDistance;

		commonData_.animator->StartAnimation(BossAnimationType::kRush, "Rush_Main", rushMaxTime_, false);
	}
}

void BossRushAttackAction::RushAttack() {
	timer_ += FpsCounter::deltaTime / rushMaxTime_;

	// 移動
	commonData_.transform.translate = Lerp(startRushPos_, endRushPos_,timer_);
	// 高さ
	commonData_.transform.translate.y = Lerp(startRushPos_.y, 2.0f, timer_);

	// 回転
	Vector3 dir = endRushPos_ - startRushPos_;
	dir.Normalize();
	commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);
	commonData_.transform.rotate.z = 0.0f;

	if (timer_ >= 1.0f) {
		isFinished_ = true;
	}
}

//===============================================================
// 待機
//===============================================================

BossWaitAction::BossWaitAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void BossWaitAction::Initialize() {
	timer_ = 0.0f;
	isFinished_ = false;

	commonData_.animator->StartAnimation(BossAnimationType::kMove, "基本移動");
}

void BossWaitAction::Update() {
	timer_ += FpsCounter::deltaTime / maxTIme_;

	if (timer_ >= 1.0f) {
		isFinished_ = true;
	}
}

void BossWaitAction::Finalize() {

}

//===============================================================
// 横断移動
//===============================================================

BossCrossMoveAction::BossCrossMoveAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void BossCrossMoveAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;
	// 開始位置
	startPos_ = commonData_.transform.translate;
	Vector3 startDir = startPos_;
	startDir.y = 0.0f;
	startDir.Normalize();

	float angle = RandomGenerator::Get(-std::numbers::pi_v<float> / 4.0f, std::numbers::pi_v<float> / 4.0f);
	Vector3 dir = RotateVectorXZ(startDir, angle);
	// 反転
	dir *= -1.0f;

	// 終盤の位置を取得
	endPos_ = dir * (commonData_.stageRadius * crossEndRatio_);

	// 最初の回転するための角度を求める
	startCurrentRotDir_ = commonData_.transform.translate;
	startCurrentRotDir_.y = 0.0f;
	startCurrentRotDir_.Normalize();
	// 最初の内の最後に向く方向
	endRotDir_ = Math::Normalize(endPos_);

	// 最終的に向く方向
	finalRotDir_ = endRotDir_ * -1.0f;

	commonData_.animator->StartAnimation(BossAnimationType::kMove, "基本移動");
}

void BossCrossMoveAction::Update() {

	timer_ += FpsCounter::deltaTime;
	timer_ = std::min(timer_, 1.0f);

	// 縦移動
	float posY = 0.0f;
	float totalCycle = timer_ * upDownCount_;
	float localTimer = std::fmodf(totalCycle, 1.0f);
	if (localTimer <= 0.5f) {
		float t = localTimer / 0.5f;
		posY = Lerp(0.0f, maxMoveHeight_, EaseInOut(t));
	} else {
		float t = (localTimer - 0.5f) / 0.5f;
		posY = Lerp(maxMoveHeight_, 0.0f, EaseInOut(t));
	}

	// 移動
	Vector3 pos = Lerp(startPos_, endPos_, EaseInOut(timer_));
	commonData_.transform.translate = pos;
	commonData_.transform.translate.y = defaultPosY_;
	commonData_.transform.translate.y += posY;

	/// 回転
	Vector3 dir = { 0,0,1 };

	// 回転の処理
	if (timer_ <= 0.2f) {
		float localT = timer_ / 0.2f;
		// 回転
		dir = Slerp(startCurrentRotDir_, endRotDir_, EaseIn(localT));
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);
	} else if (timer_ >= 0.8f) {

		float localT = (timer_ - 0.8f) / 0.2f;
		// 回転
		dir = Slerp(endRotDir_, finalRotDir_, EaseOut(localT));
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);
	}
	
	if (timer_ >= 1.0f) {
		isFinished_ = true;
	}
}

void BossCrossMoveAction::Finalize() {

}

//===============================================================
// 回転移動
//===============================================================

RotateMoveAction::RotateMoveAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void RotateMoveAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;
	
	Vector3 myDir = commonData_.transform.translate;
	myDir.y = 0.0f;
	myDir.Normalize();
	// 最初の角度
	startAngle_ = std::atan2f(myDir.z, myDir.x);

	// 回転する方向を求める
	float rotOffset = 0.0f;
	if (RandomGenerator::Get(0, 1) == 0) {
		rotOffset = std::numbers::pi_v<float> *0.5f;
	} else {
		rotOffset = -std::numbers::pi_v<float> *0.5f;
	}

	// 反対側の角度を求める
	endAngle_ = startAngle_ + rotOffset;

	// 上下移動する回数を求める
	float radius = commonData_.stageRadius + offsetStageRadius_;
	Vector3 endPos = GetXZFromAngle(endAngle_, radius, defaultPosY_);
	float angleDiff = std::fabs(endPos.Length());
	cycleCount_ = angleDiff / (commonData_.stageRadius * 0.5f);

	// 最初の回転するための角度を求める
	startCurrentRotDir_ = commonData_.transform.translate;
	startCurrentRotDir_.y = 0.0f;
	startCurrentRotDir_.Normalize();
	// 最初の内の最後に向く方向
	float angle = Lerp(startAngle_, endAngle_, 0.2f);
	Vector3 prePos = GetXZFromAngle(angle, radius, defaultPosY_);
	endRotDir_ = Math::Normalize(prePos - commonData_.transform.translate);
	// 最終的に向く方向
	angle = Lerp(startAngle_, endAngle_, 1.0f);
	prePos = GetXZFromAngle(angle, radius, defaultPosY_);
	finalRotDir_ = Math::Normalize(prePos * -1.0f);

	commonData_.animator->StartAnimation(BossAnimationType::kMove, "基本移動");
}

void RotateMoveAction::Update() {
	timer_ += FpsCounter::deltaTime / maxTime_;
	timer_ = std::min(timer_, 1.0f);

	// 角度補間
	float preAngle = angle_;
	angle_ = Lerp(startAngle_, endAngle_, timer_);

	// 半径を取得
	float radius = commonData_.stageRadius + offsetStageRadius_;

	// 縦移動
	float posY = 0.0f;
	float totalCycle = timer_ * cycleCount_;
	float localTimer = std::fmodf(totalCycle, 1.0f);

	if (localTimer <= 0.5f) {
		float t = localTimer / 0.5f;
		posY = Lerp(0.0f, maxMoveHeight_, EaseInOut(t));
	} else {
		float t = (localTimer - 0.5f) / 0.5f;
		posY = Lerp(maxMoveHeight_, 0.0f, EaseInOut(t));
	}

	// 回転移動
	commonData_.transform.translate = GetXZFromAngle(angle_, radius, defaultPosY_);
	commonData_.transform.translate.y += posY;

	/// 回転
	Vector3 dir = { 0,0,1 };

	// 回転の処理
	if (timer_ <= 0.2f) {
		float localT = timer_ / 0.2f;

		// 回転
		dir = Slerp(startCurrentRotDir_, endRotDir_, EaseIn(localT));
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

	} else if (timer_ <= 0.8f) {
		// 回転
		Vector3 prePos = GetXZFromAngle(preAngle, radius, defaultPosY_);
		Vector3 dir = commonData_.transform.translate - prePos;
		dir.Normalize();
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);
		// 保存
		startCurrentRotDir_ = dir;
	} else {
		float localT = (timer_ - 0.8f) / 0.2f;
		// 回転
		dir = Slerp(startCurrentRotDir_, finalRotDir_, EaseOut(localT));
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);
	}

	if (timer_ >= 1.0f) {
		isFinished_ = true;
	}
}

void RotateMoveAction::Finalize() {

}

//==========================================================================
// 氷柱攻撃
//==========================================================================

IceFallAttackAction::IceFallAttackAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void IceFallAttackAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;

	// 叫びモージョンに以降
	commonData_.animator->StartAnimation(BossAnimationType::kScream, "Scream", maxTime_, false);
}

void IceFallAttackAction::Update() {
	timer_ += FpsCounter::deltaTime / maxTime_;


	if (timer_ >= 1.0f) {
		isFinished_ = true;
	}
}

void IceFallAttackAction::Finalize() {

}

//==========================================================================
// 風攻撃
//==========================================================================

WindAttackAction::WindAttackAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void WindAttackAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;
	state_ = State::kIn;

	// 現在の方向を求める
	startCurrentRotDir_ = commonData_.transform.translate;
	startCurrentRotDir_.y = 0.0f;
	startCurrentRotDir_.Normalize();
	startCurrentRotDir_ *= -1.0f;

	float angle = std::numbers::pi_v<float> / 4.0f;
	startRotDir_ = RotateVectorXZ(startCurrentRotDir_, angle);
	endRotDir_ = RotateVectorXZ(startCurrentRotDir_, -angle);

	commonData_.animator->StartAnimation(BossAnimationType::kBreath, "IceBreath_Prepare", inMaxTime_,false);
}

void WindAttackAction::Update() {

	switch (state_)
	{
	case WindAttackAction::State::kIn: {
		timer_ += FpsCounter::deltaTime / inMaxTime_;

		// 回転
		Vector3 dir = Slerp(startCurrentRotDir_, startRotDir_, timer_);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

		if (timer_ >= 1.0f) {
			state_ = State::kMain;
			timer_ = 0.0f;

			commonData_.animator->StartAnimation(BossAnimationType::kBreath, "IceBreath_Main", mainMaxTime_, false);
		}
		break;
	}

	case WindAttackAction::State::kMain: {
		timer_ += FpsCounter::deltaTime / mainMaxTime_;

		// 回転
		Vector3 dir = Slerp(startRotDir_, endRotDir_, timer_);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

		if (timer_ >= 1.0f) {
			state_ = State::kOut;
			timer_ = 0.0f;
			commonData_.animator->StartAnimation(BossAnimationType::kMove, "基本移動");
		}
		break;
	}


	case WindAttackAction::State::kOut: {
		timer_ += FpsCounter::deltaTime / outMaxTime_;

		// 回転
		Vector3 dir = Slerp(endRotDir_, startCurrentRotDir_, timer_);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

		if (timer_ >= 1.0f) {
			isFinished_ = true;
		}
		break;
	}
	}
}

void WindAttackAction::Finalize() {

}

// ヘルパー関数
namespace {

	Vector3 GetXZFromAngle(float angle, float radius, float posY) {
		return  Vector3(std::cosf(angle) * radius, posY, std::sinf(angle) * radius);
	}

	Vector3 RotateVectorXZ(Vector3 dir, float angle) {
		float cos = std::cosf(angle);
		float sin = std::sinf(angle);
		return Vector3(dir.x * cos - dir.z * sin, 0.0f, dir.x * sin + dir.z * cos );
	}
}



