#pragma once
#include <optional>

// 状態
enum class ScenePhase {
    kTitle,    // タイトル
    kTutorial, // チュートリアル
    kPlay,     // プレイ
    kGameOver, // ゲームオーバー
    kClear,    // クリア
    kPause,    // ポーズ

    kMaxCount
};

// 各フェーズで共通データ
struct PhaseCommonData {

    // 現在のフェーズ
    ScenePhase currentPhase = ScenePhase::kTitle;

    // リクエスト
    std::optional<ScenePhase> requestPhase = std::nullopt;

    // プレイ時間
    float playTime_ = 0.0f;
};

class IScenePhase {
public:
    IScenePhase(PhaseCommonData& commonData) : commonData_(commonData) {}
    virtual ~IScenePhase() = default;

    /// <summary>
    /// 入り
    /// </summary>
    virtual void Enter() = 0;

    /// <summary>
    /// 更新処理
    /// </summary>
    virtual void Update() = 0;

    /// <summary>
    /// 終わり
    /// </summary>
    virtual void Exit() = 0;

    /// <summary>
    /// 終了したかどうか
    /// </summary>
    bool IsFinished() const { return isFinished_; }

protected:
    PhaseCommonData& commonData_;
    bool isFinished_ = false;
};