#pragma once

// 時間を管理する
class TimeController {
public:
	TimeController();

	// 更新処理
	void Update();

public:

	// 時間を止める（停止中に呼ばれた場合は、残り時間が長い方を優先する）
	void StartStopTime(float stopMaxTime);

	// 時間が止まっているか
	bool IsStopTimeActive() const { return isStopTimeActive_; }

	// リセット（ゲーム用の時間を通常の速さに戻す）
	void Reset();

private:
	bool isStopTimeActive_ = false;
	float stopMaxTime_ = 0.0f;
	float timer_ = 0.0f;
};