#include "BossBattleAction.h"
#include <numbers>
#include "FPSCounter.h"
#include "MyMath.h"
#include "EasingManager.h"
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
	float angle = Lerp(startAngle_, endAngle_, timer_);

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
	commonData_.transform.translate = Vector3(std::cosf(angle) * radius, defaultPosY_, std::sinf(angle) * radius);

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
	}
}

void BossRushAttackAction::RushAttack() {
	timer_ += FpsCounter::deltaTime / rushMaxTime_;

	// 移動
	commonData_.transform.translate = Lerp(startRushPos_, endRushPos_,timer_);

	// 高さ
	commonData_.transform.translate.y = Lerp(startRushPos_.y, 2.0f, timer_);

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

}

void BossCrossMoveAction::Update() {
	
}

void BossCrossMoveAction::Finalize() {

}

