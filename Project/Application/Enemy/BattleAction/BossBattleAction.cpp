#include "BossBattleAction.h"
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

	Vector3 myDir = commonData_.transform.translate;
	myDir.y = 0.0f;
	myDir.Normalize();
	// 最初の角度
	startAngle_ = std::atan2f(myDir.z, myDir.x);

	Vector3 playerDir = *commonData_.playerPos;
	playerDir.y = 0;
	playerDir.Normalize();
	playerDir = playerDir * -1.0f;
	// 最後の角度
	endAngle_ = std::atan2f(playerDir.z, playerDir.x);

	// 突進の位置を取得
	startRushPos_ = playerDir * commonData_.stageRadius;
	startRushPos_.y = defaultPosY_;
	endRushPos_ = myDir * commonData_.stageRadius;
	endRushPos_.y = defaultPosY_;
}

void BossRushAttackAction::Update() {

	timer_ += FpsCounter::deltaTime / rotateMoveMaxTime_;

	float angle = Math::LerpShortAngle(startAngle_, endAngle_, timer_);

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
		isFinished_ = true;
	}

}

void BossRushAttackAction::Finalize() {

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