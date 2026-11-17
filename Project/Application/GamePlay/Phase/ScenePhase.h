#pragma once
#include "Application/GamePlay/IScenePhase.h"
#include "Application/Utils/Timer.h"
#include "Application/GamePlay/OptionSettings.h"

// 前方宣言
class Player;
class BossEnemy;
class CameraController;
class TitleUIManager;
class PlayUIManager;
class GameOverUIManager;
class ClearUIManager;
class PauseUIManager;
class EnterMovieUIManager;
class OptionUIManager;

// タイトル
class TitlePhase : public IScenePhase {
public:
    TitlePhase(PhaseCommonData& commonData, CameraController* cameraController, TitleUIManager* titleUIManager, Player* player);
    ~TitlePhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    CameraController* cameraController_ = nullptr;

    TitleUIManager* titleUIManager_ = nullptr;

    Player* player_ = nullptr;
};

// チュートリアル
class TutorialPhase : public IScenePhase {
public:
    TutorialPhase(PhaseCommonData& commonData, CameraController* cameraController, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager,
        EnterMovieUIManager* enterMovieUIManager);
    ~TutorialPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    // ボス
    BossEnemy* bossEnemy_ = nullptr;

    // プレイヤー
    Player* player_ = nullptr;

    // UI
    PlayUIManager* playUIManager_ = nullptr;

    // カメラ管理処理
    CameraController* cameraController_ = nullptr;

    // 入りのムービーのUI
    EnterMovieUIManager* enterMovieUIManager_ = nullptr;
    // ボスの名前を表示したか
    bool isPlayBossName_ = false;
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

    // カメラ管理処理
    CameraController* cameraController_ = nullptr;

    // プレイ時間を計測
    Timer playTimer_;
};

// ポーズシーン
class PausePhase : public IScenePhase {
public:
    PausePhase(PhaseCommonData& commonData, PauseUIManager* pauseUIManager);
    ~PausePhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    PauseUIManager* pauseUIManager_ = nullptr;

    int32_t selectNum_ = 0;

    // ポーズを閉じた時に戻るフェーズ
    ScenePhase returnPhase_ = ScenePhase::kPlay;
};

// ゲームオーバーシーン
class GameOverPhase : public IScenePhase {
public:
    GameOverPhase(PhaseCommonData& commonData, GameOverUIManager* gameOverUIManager);
    ~GameOverPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    GameOverUIManager* gameOverUIManager_ = nullptr;


};

// クリア
class ClearPhase : public IScenePhase {
public:
    ClearPhase(PhaseCommonData& commonData, ClearUIManager* clearUIManager);
    ~ClearPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    ClearUIManager* clearUIManager_ = nullptr;


};

// オプション
class OptionPhase : public IScenePhase {
public:
    // 押し続けた時に一定間隔で入力を受け付ける
    struct RepeatInput {
        bool isPrePush = false;
        float timer = 0.0f;
    };

public:
    OptionPhase(PhaseCommonData& commonData, OptionUIManager* optionUIManager, CameraController* cameraController);
    ~OptionPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    OptionUIManager* optionUIManager_ = nullptr;
    CameraController* cameraController_ = nullptr;

    // 設定
    OptionSettings settings_;

    // 戻り先のフェーズ
    ScenePhase returnPhase_ = ScenePhase::kTitle;

    // 選択中の項目
    int32_t selectNum_ = 0;

    // 上下左右の入力状態
    RepeatInput upInput_;
    RepeatInput downInput_;
    RepeatInput leftInput_;
    RepeatInput rightInput_;

    // 押し続けてからリピートが始まるまでの時間
    float kRepeatDelay_ = 0.4f;
    // リピートの間隔
    float kRepeatInterval_ = 0.1f;

    // SEの確認用の音
    uint32_t seCheckHandle_ = 0;

private:
    // 押した瞬間、または押し続けている間一定間隔でtrueを返す
    bool IsRepeatInput(const std::string& commandName, RepeatInput& state);

    // 選択中の項目の値を変更する
    void ChangeValue(int32_t dir);

    // 設定を音量、カメラ、UIに反映する
    void ApplySettings();
};