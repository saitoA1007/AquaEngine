#pragma once
#include "Application/GamePlay/IScenePhase.h"

// 前方宣言
class Player;
class PlayUIManager;
class BossEnemy;

// タイトル
class TitlePhase : public IScenePhase {
public:
    TitlePhase(PhaseCommonData& commonData);
    ~TitlePhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:


};

// プレイ
class PlayPhase : public IScenePhase {
public:
    PlayPhase(PhaseCommonData& commonData, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager);
    ~PlayPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    // プレイヤー
    Player* player_ = nullptr;

    // ボス
    BossEnemy* bossEnemy_ = nullptr;

    // UI
    PlayUIManager* playUIManager_ = nullptr;
};

// クリア
class ClearPhase : public IScenePhase {
public:
    ClearPhase(PhaseCommonData& commonData);
    ~ClearPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
};