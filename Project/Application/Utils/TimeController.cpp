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

void TimeController::Reset() {
	isStopTimeActive_ = false;
	timer_ = 0.0f;
	// ゲーム用の時間を通常の速さに戻す
	FpsCounter::SetTimeScale(1.0f);
}
