#pragma once
#include "IBossBattleAction.h"

// 突進攻撃
class BossRushAttackAction : public IBossBattleAction {
public:
    BossRushAttackAction(BossBattleStateCommonData& commonData);
    ~BossRushAttackAction() = default;

    void Initialize() override;
    void Update() override;
    void Finalize() override;

private:
    BossBattleStateCommonData& commonData_;

};

// 待機
class BossWaitAction : public IBossBattleAction {
public:
    BossWaitAction(BossBattleStateCommonData& commonData);
    ~BossWaitAction() = default;

    void Initialize() override;
    void Update() override;
    void Finalize() override;

private:
    BossBattleStateCommonData& commonData_;

};