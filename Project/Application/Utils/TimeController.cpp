#include "pch.h"
#include "TimeController.h"
#include "FPSCounter.h"
using namespace GameEngine;

TimeController::TimeController() {

}

void TimeController::Update() {

	// 時間を止める
	if (isStopTimeActive_) {

		timer_ += FpsCounter::GetDeltaTime() / stopMaxTime_;

		// タイムスケールを0にしてゲーム用のデルタ時間を止める
		FpsCounter::SetTimeScale(0.0f);

		if (timer_ >= 1.0f) {
			// 時間の停止を終了して元に戻す
			Reset();
		}
	}
}

void TimeController::StartStopTime(float stopMaxTime) {
	// 0以下の時間では止めない（0除算を防ぐ）
	if (stopMaxTime <= 0.0f) { return; }

	// すでに止まっている場合、残り時間の方が長ければそのままにする
	// （ポーズ中のヒットストップは無視し、ヒットストップ中のポーズは優先する）
	if (isStopTimeActive_) {
		float remainingTime = stopMaxTime_ * (1.0f - timer_);
		if (stopMaxTime <= remainingTime) { return; }
	}

	stopMaxTime_ = stopMaxTime;
	timer_ = 0.0f;
	isStopTimeActive_ = true;

	// 呼ばれたフレームからすぐに止める
	FpsCounter::SetTimeScale(0.0f);
}

void TimeController::Reset() {
	isStopTimeActive_ = false;
	timer_ = 0.0f;
	// ゲーム用の時間を通常の速さに戻す
	FpsCounter::SetTimeScale(1.0f);
}
