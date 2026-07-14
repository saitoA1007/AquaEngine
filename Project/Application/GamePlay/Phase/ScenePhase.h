#pragma once
#include "Application/GamePlay/IScenePhase.h"
#include "Application/Utils/Timer.h"

// 前方宣言
class Player;
class PlayUIManager;
class BossEnemy;
class CameraController;

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

// チュートリアル
class TutorialPhase : public IScenePhase {
public:
    TutorialPhase(PhaseCommonData& commonData, CameraController* cameraController, BossEnemy* bossEnemy, PlayUIManager* playUIManager);
    ~TutorialPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    // ボス
    BossEnemy* bossEnemy_ = nullptr;

    // UI
    PlayUIManager* playUIManager_ = nullptr;

    // カメラ管理処理
    CameraController* cameraController_ = nullptr;

};

// プレイ
class PlayPhase : public IScenePhase {
public:
    PlayPhase(PhaseCommonData& commonData, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager, CameraController* cameraController);
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
    bool isBarActive_ = false;

    // カメラ管理処理
    CameraController* cameraController_ = nullptr;

    // プレイ時間を計測
    Timer playTimer_;
};

// ポーズシーン
class PausePhase : public IScenePhase {
public:
    PausePhase(PhaseCommonData& commonData);
    ~PausePhase() = default;

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