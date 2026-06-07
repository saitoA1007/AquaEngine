#pragma once
#include "Application/GamePlay/IScenePhase.h"

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
    PlayPhase(PhaseCommonData& commonData);
    ~PlayPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:

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