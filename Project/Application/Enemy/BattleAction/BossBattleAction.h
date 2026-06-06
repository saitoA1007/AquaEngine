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
    // 回転移動時間
    float rotateMoveMaxTime_ = 1.0f;

private:

    // 回転移動の角度
    float startAngle_ = 0.0f;
    float endAngle_ = 0.0f;

    // 突進の位置
    Vector3 startRushPos_;
    Vector3 endRushPos_;

    float defaultPosY_ = 5.0f;

    float timer_ = 0.0f;
};

// 待機
class BossWaitAction : public IBossBattleAction {
public:
    BossWaitAction(BossBattleStateCommonData& commonData);
    ~BossWaitAction() = default;

    void Initialize() override;
    void Update() override;
    void Finalize() override;

    //void RegisterParameter([[maybe_unused]] GameEngine::DebugParameter* param) override;

private:

    float timer_ = 0.0f;
    // 待機時間
    float maxTIme_ = 1.0f;
};