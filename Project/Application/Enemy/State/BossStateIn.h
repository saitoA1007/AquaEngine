#pragma once
#include "Application/Enemy/IBossState.h"

class BossStateIn : public IBossState {

public:
	BossStateIn(BossStateCommonData& commonData);
	~BossStateIn() = default;

	/// <summary>
	/// 入りの処理
	/// </summary>
	void Enter() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// 終わりの処理
	/// </summary>
	void Exit() override;

public:

	/// <summary>
	/// 入りの演出を飛ばして、終わった状態にする
	/// </summary>
	void Skip();

private:
	BossStateCommonData& stateCommonData_;

	Vector3 startPos_ = {0.0f,0.0f,0.0f};
	Vector3 endPos_ = {0.0f,15.0f,0.0f};
	float moveHeight_ = 12.0f;
	uint32_t cycleCount_ = 3;

	float timer_ = 0.0f;

	float delayMaxTime_ = 1.0f;
	float maxInTime_ = 2.0f;
	float maxWaitTime_ = 2.0f;

	bool delayBreakEgg_ = false;

	bool isMove_ = true;
};