#pragma once
#include "Application/Enemy/IBossState.h"

class BossStateBattle : public IBossState {
public:
	BossStateBattle(BossStateCommonData& commonData);
	~BossStateBattle() = default;

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

private:
	BossStateCommonData& stateCommonData_;



};